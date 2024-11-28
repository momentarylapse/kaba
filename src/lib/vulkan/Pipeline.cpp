//
//  Pipeline.cpp
//  3
//
//  Created by <author> on 06/02/2019.
//
//

#if HAS_LIB_VULKAN


#include "Pipeline.h"
#include "vulkan.h"
#include "common.h"
#include "Shader.h"
#include "helper.h"
#include "RenderPass.h"
#include "VertexBuffer.h"
#include "../base/sort.h"
#include "../os/msg.h"
#include "../math/rect.h"

namespace vulkan {

VkPrimitiveTopology parse_topology(const string &t) {
	if (t == "points")
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	if (t == "lines")
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	if (t == "line-strip")
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	if (t == "triangles")
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	if (t == "triangles-fan")
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	msg_error("invalid topology: " + t);
	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}


Array<VkPipelineShaderStageCreateInfo> create_shader_stages(Shader *shader) {
	Array<VkPipelineShaderStageCreateInfo> shader_stages;
	for (auto &m: shader->modules) {
		VkPipelineShaderStageCreateInfo shader_stage_info = {};
		shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_info.stage = m.stage;
		shader_stage_info.module = m.module;
		shader_stage_info.pName = "main";
		shader_stages.add(shader_stage_info);
	}
	shader_stages = base::sorted(shader_stages, [](const auto& a, const auto& b) {
		return (int)a.stage <= (int)b.stage;
	});
	return shader_stages;
}

VkPipelineLayout create_pipeline_layout(int push_size, const Array<VkDescriptorSetLayout> &dset_layouts) {
	VkPipelineLayoutCreateInfo info = {};
	VkPushConstantRange pci = {VK_SHADER_STAGE_ALL, 0, (uint32_t)push_size};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	if (push_size > 0) {
		pci.stageFlags = VK_SHADER_STAGE_ALL; //VK_SHADER_STAGE_VERTEX_BIT /*| VK_SHADER_STAGE_GEOMETRY_BIT*/ | VK_SHADER_STAGE_FRAGMENT_BIT;
		pci.offset = 0;
		pci.size = push_size;
		info.pushConstantRangeCount = 1;
		info.pPushConstantRanges = &pci;
	}
	info.setLayoutCount = dset_layouts.num;
	info.pSetLayouts = &dset_layouts[0];
	if (verbosity >= 2)
		msg_write(format("create pipeline with %d layouts, %d push size", dset_layouts.num, push_size));

	VkPipelineLayout layout = VK_NULL_HANDLE;
	if (vkCreatePipelineLayout(default_device->device, &info, nullptr, &layout) != VK_SUCCESS)
		throw Exception("failed to create pipeline layout!");
	return layout;
}

BasePipeline::BasePipeline(VkPipelineBindPoint bp, Shader *s) {
	bind_point = bp;
	shader = s;
	descr_layouts = shader->descr_layouts;

	shader_stages = create_shader_stages(shader);

	layout = create_pipeline_layout(shader->push_size, descr_layouts);
}

BasePipeline::BasePipeline(VkPipelineBindPoint bp, const Array<VkDescriptorSetLayout> &dset_layouts) {
	bind_point = bp;
	descr_layouts = dset_layouts;
	layout = create_pipeline_layout(0, dset_layouts);
}

BasePipeline::~BasePipeline() {
	destroy();
	if (layout)
		vkDestroyPipelineLayout(default_device->device, layout, nullptr);
}

void BasePipeline::destroy() {
	if (pipeline)
		vkDestroyPipeline(default_device->device, pipeline, nullptr);
	pipeline = nullptr;
}

Array<VkVertexInputAttributeDescription> parse_attr_descr(const string &format);
VkVertexInputBindingDescription parse_binding_descr(const string &format);

GraphicsPipeline::GraphicsPipeline(Shader *_shader, RenderPass *_render_pass, int _subpass, const string &topology, VertexBuffer *vb) : GraphicsPipeline(_shader, _render_pass, _subpass, topology, vb->binding_description, vb->attribute_descriptions) {}

GraphicsPipeline::GraphicsPipeline(Shader *_shader, RenderPass *_render_pass, int _subpass, const string &topology, const string &format) : GraphicsPipeline(_shader, _render_pass, _subpass, topology, parse_binding_descr(format), parse_attr_descr(format)) {}

GraphicsPipeline::GraphicsPipeline(Shader *_shader, RenderPass *_render_pass, int _subpass, const string &topology, VkVertexInputBindingDescription _binding_description, const Array<VkVertexInputAttributeDescription> &_attribute_descriptions) : BasePipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, _shader) {
	render_pass = _render_pass;
	subpass = _subpass;

	binding_description = _binding_description;
	attribute_descriptions = _attribute_descriptions;
	vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = attribute_descriptions.num;
	vertex_input_info.pVertexAttributeDescriptions = &attribute_descriptions[0];

	input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = parse_topology(topology);
	input_assembly.primitiveRestartEnable = VK_FALSE;
#ifdef OS_MAC
	input_assembly.primitiveRestartEnable = VK_TRUE; // molten vk/metal seem to lack the feature of "turning this off" (O_O)'
#endif

	for (int i=0; i<render_pass->num_color_attachments(subpass); i++) {
		VkPipelineColorBlendAttachmentState a = {};
		a.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		a.blendEnable = VK_FALSE;
		color_blend_attachments.add(a);
	}

	color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = color_blend_attachments.num;
	color_blending.pAttachments = &color_blend_attachments[0];
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;


	rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	//rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizer.lineWidth = 1;
	//rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	depth_stencil = {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;

	dynamic_states.add(VK_DYNAMIC_STATE_VIEWPORT);
	set_viewport(rect(0, 400, 0, 400)); // always override dynamically!

	rebuild();
}

GraphicsPipeline::~GraphicsPipeline() {
	destroy();
}



void GraphicsPipeline::disable_blend() {
	color_blend_attachments[0].blendEnable = VK_FALSE;
}

void GraphicsPipeline::set_blend(Alpha src, Alpha dst) {
	color_blend_attachments[0].blendEnable = VK_TRUE;
	color_blend_attachments[0].colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachments[0].srcColorBlendFactor = (VkBlendFactor)src;
	color_blend_attachments[0].dstColorBlendFactor = (VkBlendFactor)dst;
	color_blend_attachments[0].alphaBlendOp = VK_BLEND_OP_MAX;
	color_blend_attachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
}

void GraphicsPipeline::set_blend(float alpha) {
	color_blend_attachments[0].blendEnable = VK_TRUE;
	color_blend_attachments[0].colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
	color_blend_attachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	color_blending.blendConstants[0] = alpha;
	color_blending.blendConstants[1] = alpha;
	color_blending.blendConstants[2] = alpha;
	color_blending.blendConstants[3] = alpha;
	color_blend_attachments[0].alphaBlendOp = VK_BLEND_OP_MAX;
	color_blend_attachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
}

void GraphicsPipeline::set_line_width(float line_width) {
	rasterizer.lineWidth = line_width;
}

void GraphicsPipeline::set_wireframe(bool wireframe) {
	if (wireframe) {
		rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	} else {
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	}
}

void GraphicsPipeline::set_z(bool test, bool write) {
	depth_stencil.depthTestEnable = test ? VK_TRUE : VK_FALSE;
	depth_stencil.depthWriteEnable = write ? VK_TRUE : VK_FALSE;

	if (test and !write)
		depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
}

void GraphicsPipeline::set_viewport(const rect &r) {
	viewport = {};
	viewport.x = r.x1;
	viewport.y = r.y1;
	viewport.width = r.width();
	viewport.height = r.height();
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
}

void GraphicsPipeline::set_culling(CullMode mode) {
	rasterizer.cullMode = (VkCullModeFlags)mode;
}

VkDynamicState parse_dynamic_state(const string &d) {
	if (d == "viewport")
		return VK_DYNAMIC_STATE_VIEWPORT;
	if (d == "scissor")
		return VK_DYNAMIC_STATE_SCISSOR;
	if (d == "linewidth")
		return VK_DYNAMIC_STATE_LINE_WIDTH;
	msg_error("unknown dynamic state: " + d);
	return VK_DYNAMIC_STATE_MAX_ENUM;
}

void GraphicsPipeline::set_dynamic(const Array<string> &_dynamic_states) {
	for (string &d: _dynamic_states) {
		auto ds = parse_dynamic_state(d);
		if (ds != VK_DYNAMIC_STATE_VIEWPORT)
			dynamic_states.add(ds);
	}
}

void GraphicsPipeline::rebuild() {
	destroy();

	// sometimes a dummy scissor is required!
	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = {(unsigned)1000000, (unsigned)1000000};
	//scissor.extent = {(unsigned)width, (unsigned)height};

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;



	VkPipelineDynamicStateCreateInfo dynamic_state = {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = dynamic_states.num;
	dynamic_state.pDynamicStates = &dynamic_states[0];

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = shader_stages.num;
	pipeline_info.pStages = &shader_stages[0];
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.layout = layout;
	pipeline_info.renderPass = render_pass->render_pass;
	pipeline_info.subpass = subpass;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.pDynamicState = &dynamic_state;

	if (vkCreateGraphicsPipelines(default_device->device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
		throw Exception("failed to create graphics pipeline!");
}



ComputePipeline::ComputePipeline(Shader *shader) : BasePipeline(VK_PIPELINE_BIND_POINT_COMPUTE, shader) {
	if (verbosity >= 2)
		msg_write("creating compute pipeline...");
	
	VkComputePipelineCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.layout = layout;
	info.stage = shader_stages[0];

	if (vkCreateComputePipelines(default_device->device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS)
		throw Exception("failed to create compute pipeline!");
}



RayPipeline::RayPipeline(const string &dset_layouts, const Array<Shader*> &shaders, int recursion_depth) :
		BasePipeline(VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, DescriptorSet::parse_bindings(dset_layouts)),
		sbt(default_device)
{
	if (verbosity >= 2)
		msg_write("creating RTX pipeline...");

	create_groups(shaders);


	VkRayTracingPipelineCreateInfoNV info = {};
	info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	info.groupCount = groups.num;
	info.pGroups = &groups[0];
	info.stageCount = shader_stages.num;
	info.pStages = &shader_stages[0];
	info.maxRecursionDepth = recursion_depth;
	info.layout = layout;
	info.basePipelineHandle = VK_NULL_HANDLE;
	info.basePipelineIndex = 0;

	if (_vkCreateRayTracingPipelinesNV(default_device->device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS) {
		throw Exception("failed to create graphics pipeline!");
	}
	if (verbosity >= 2)
		msg_write("...done");
}

void RayPipeline::create_groups(const Array<Shader*> &shaders) {

	VkPipelineShaderStageCreateInfo stage = {};
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.pName = "main";

	// ray gen
	stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_NV;
	stage.module = shaders[0]->get_module(VK_SHADER_STAGE_RAYGEN_BIT_NV);
	shader_stages.add(stage);

	VkRayTracingShaderGroupCreateInfoNV group_info = {};
	group_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	group_info.closestHitShader = VK_SHADER_UNUSED_NV;
	group_info.anyHitShader = VK_SHADER_UNUSED_NV;
	group_info.intersectionShader = VK_SHADER_UNUSED_NV;
	groups.add(group_info);

    // hit groups
    for (auto *s: shaders.sub_ref(1)) {
        group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
        group_info.generalShader = VK_SHADER_UNUSED_NV;
        group_info.closestHitShader = VK_SHADER_UNUSED_NV;
        group_info.anyHitShader = VK_SHADER_UNUSED_NV;
        group_info.intersectionShader = VK_SHADER_UNUSED_NV;

    	Array<VkPipelineShaderStageCreateInfo> stages;
    	if (s->get_module(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV)) {
    		group_info.closestHitShader = shader_stages.num + stages.num;
    		stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
    		stage.module = s->get_module(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    		stages.add(stage);
    	}
    	if (s->get_module(VK_SHADER_STAGE_ANY_HIT_BIT_NV)) {
    		group_info.anyHitShader = shader_stages.num + stages.num;
    		stage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_NV;
    		stage.module = s->get_module(VK_SHADER_STAGE_ANY_HIT_BIT_NV);
    		stages.add(stage);
    	}
    	//VK_SHADER_STAGE_INTERSECTION_BIT_NV
    	shader_stages.append(stages);
    	groups.add(group_info);
    }
    miss_group_offset = groups.num;

    // miss groups
    for (auto *s: shaders.sub_ref(1))
    	if (s->get_module(VK_SHADER_STAGE_MISS_BIT_NV)) {
			group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
			group_info.generalShader = shader_stages.num;
			group_info.closestHitShader = VK_SHADER_UNUSED_NV;
			group_info.anyHitShader = VK_SHADER_UNUSED_NV;
			group_info.intersectionShader = VK_SHADER_UNUSED_NV;

    		stage.stage = VK_SHADER_STAGE_MISS_BIT_NV;
    		stage.module = s->get_module(VK_SHADER_STAGE_MISS_BIT_NV);
    		shader_stages.add(stage);
    		groups.add(group_info);
    	}
}

void RayPipeline::create_sbt() {
	if (verbosity >= 2)
		msg_write("SBT");
	const size_t sbt_size = groups.num * default_device->ray_tracing_properties.shaderGroupHandleSize;

	sbt.create(sbt_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	void* mem = sbt.map();
	if (_vkGetRayTracingShaderGroupHandlesNV(default_device->device, pipeline, 0, groups.num, sbt_size, mem))
		throw Exception("vkGetRayTracingShaderGroupHandlesNV");
	sbt.unmap();
	if (verbosity >= 2)
		msg_write("   ok");
}

};

#endif

