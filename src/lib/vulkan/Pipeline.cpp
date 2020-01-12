//
//  Pipeline.cpp
//  3
//
//  Created by <author> on 06/02/2019.
//
//

#if HAS_LIB_VULKAN


#include "Pipeline.h"
#include "Shader.h"
#include "helper.h"
#include "RenderPass.h"
#include "VertexBuffer.h"
#include <iostream>

#include "../math/rect.h"

namespace vulkan {

	Array<Pipeline*> pipelines;



	VkVertexInputBindingDescription create_binding_description(int num_textures) {
		VkVertexInputBindingDescription bd = {};
		bd.binding = 0;
		bd.stride = 2 * sizeof(vector) + num_textures * 2 * sizeof(float);
		bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bd;
	}

	Array<VkVertexInputAttributeDescription> create_attribute_descriptions(int num_textures) {
		Array<VkVertexInputAttributeDescription> ad;
		ad.resize(2 + num_textures);

		// position
		ad[0].binding = 0;
		ad[0].location = 0;
		ad[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		ad[0].offset = 0;

		// normal
		ad[1].binding = 0;
		ad[1].location = 1;
		ad[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		ad[1].offset = sizeof(vector);

		for (int i=0; i<num_textures; i++) {
			ad[i+2].binding = 0;
			ad[i+2].location = 2;
			ad[i+2].format = VK_FORMAT_R32G32_SFLOAT;
			ad[i+2].offset = 2 * sizeof(vector) + i * 2 * sizeof(float);
		}

		return ad;
	}


Pipeline::Pipeline(Shader *_shader, RenderPass *_render_pass, int _subpass, int num_textures) {
	shader = _shader;
	render_pass = _render_pass;
	subpass = _subpass;
	descr_layouts = shader->descr_layouts;
	pipeline = nullptr;
	layout = nullptr;


	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = shader->vert_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo geom_shader_stage_info = {};
	geom_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geom_shader_stage_info.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geom_shader_stage_info.module = shader->geom_module;
	geom_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = shader->frag_module;
	frag_shader_stage_info.pName = "main";

	shader_stages = {vert_shader_stage_info, frag_shader_stage_info};
	if (shader->geom_module) {
		shader_stages.add(geom_shader_stage_info);
		//shader_stages = {vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo};
	} else {
		//shader_stages = {vertShaderStageInfo, fragShaderStageInfo};
	}

	binding_description = create_binding_description(num_textures);
	attribute_descriptions = create_attribute_descriptions(num_textures);
	vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = attribute_descriptions.num;
	vertex_input_info.pVertexAttributeDescriptions = &attribute_descriptions[0];

	input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = shader->topology;
	input_assembly.primitiveRestartEnable = VK_FALSE;

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
	rasterizer.lineWidth = 1;
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

	pipelines.add(this);
}

Pipeline::~Pipeline() {
	destroy();

	for (int i=0; i<pipelines.num; i++)
		if (pipelines[i] == this)
			pipelines.erase(i);
}



void Pipeline::__init__(Shader *_shader, RenderPass *_render_pass, int _subpass, int num_textures) {
	new(this) Pipeline(_shader, _render_pass, _subpass, num_textures);
}

void Pipeline::__delete__() {
	this->~Pipeline();
}

void Pipeline::disable_blend() {
	color_blend_attachments[0].blendEnable = VK_FALSE;
}

void Pipeline::set_blend(VkBlendFactor src, VkBlendFactor dst) {
	color_blend_attachments[0].blendEnable = VK_TRUE;
	color_blend_attachments[0].colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachments[0].srcColorBlendFactor = src;
	color_blend_attachments[0].dstColorBlendFactor = dst;
	color_blend_attachments[0].alphaBlendOp = VK_BLEND_OP_MAX;
	color_blend_attachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
}

void Pipeline::set_blend(float alpha) {
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

void Pipeline::set_line_width(float line_width) {
	rasterizer.lineWidth = line_width;
}

void Pipeline::set_wireframe(bool wireframe) {
	if (wireframe) {
		rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	} else {
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	}
}

void Pipeline::set_z(bool test, bool write) {
	depth_stencil.depthTestEnable = test ? VK_TRUE : VK_FALSE;
	depth_stencil.depthWriteEnable = write ? VK_TRUE : VK_FALSE;
}

void Pipeline::set_viewport(const rect &r) {
	viewport = {};
	viewport.x = r.x1;
	viewport.y = r.y1;
	viewport.width = r.width();
	viewport.height = r.height();
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
}

void Pipeline::set_culling(int mode) {
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	if (mode == 0)
		rasterizer.cullMode = VK_CULL_MODE_NONE;
	if (mode == -1)
		rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
}

VkDynamicState parse_dynamic_state(const string &d) {
	if (d == "viewport")
		return VK_DYNAMIC_STATE_VIEWPORT;
	if (d == "scissor")
		return VK_DYNAMIC_STATE_SCISSOR;
	if (d == "linewidth")
		return VK_DYNAMIC_STATE_LINE_WIDTH;
	std::cerr << "unknown dynamic state: " << d.c_str() << "\n";
	return VK_DYNAMIC_STATE_MAX_ENUM;
}

void Pipeline::set_dynamic(const Array<string> &_dynamic_states) {
	for (string &d: _dynamic_states) {
		auto ds = parse_dynamic_state(d);
		if (ds != VK_DYNAMIC_STATE_VIEWPORT)
			dynamic_states.add(ds);
	}
}

void Pipeline::rebuild() {
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


	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	VkPushConstantRange pci = {0};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	if (shader->push_size > 0) {
		pci.stageFlags = VK_SHADER_STAGE_VERTEX_BIT /*| VK_SHADER_STAGE_GEOMETRY_BIT*/ | VK_SHADER_STAGE_FRAGMENT_BIT;
		pci.offset = 0;
		pci.size = shader->push_size;
		pipeline_layout_info.pushConstantRangeCount = 1;
		pipeline_layout_info.pPushConstantRanges = &pci;
	} else {
		pipeline_layout_info.pushConstantRangeCount = 0;
	}
	pipeline_layout_info.setLayoutCount = descr_layouts.num;
	pipeline_layout_info.pSetLayouts = &descr_layouts[0];
	std::cout << "create pipeline with " << descr_layouts.num << " layouts, " << shader->push_size << " push size\n";

	if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

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

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

void Pipeline::destroy() {
	if (pipeline)
		vkDestroyPipeline(device, pipeline, nullptr);
	if (layout)
		vkDestroyPipelineLayout(device, layout, nullptr);
	pipeline = nullptr;
	layout = nullptr;
}

};

#endif

