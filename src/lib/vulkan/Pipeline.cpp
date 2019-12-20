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

namespace vulkan {

	std::vector<Pipeline*> pipelines;


Pipeline::Pipeline(Shader *_shader, RenderPass *_render_pass) {
	shader = _shader;
	render_pass = _render_pass;
	descr_layouts = shader->descr_layouts;


	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = shader->vert_module;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo geomShaderStageInfo = {};
	geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geomShaderStageInfo.module = shader->geom_module;
	geomShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = shader->frag_module;
	fragShaderStageInfo.pName = "main";

	shader_stages = {vertShaderStageInfo, fragShaderStageInfo};
	if (shader->geom_module) {
		shader_stages.push_back(geomShaderStageInfo);
		//shader_stages = {vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo};
	} else {
		//shader_stages = {vertShaderStageInfo, fragShaderStageInfo};
	}

	binding_description = Vertex1::binding_description();
	attribute_descriptions = Vertex1::attribute_descriptions();
	vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

	input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = shader->topology;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	color_blend_attachment = {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;

	color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
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
}

Pipeline::~Pipeline() {
	destroy();
}



void Pipeline::__init__(Shader *_shader, RenderPass *_render_pass) {
	new(this) Pipeline(_shader, _render_pass);
}

void Pipeline::__delete__() {
	this->~Pipeline();
}

void Pipeline::disable_blend() {
	color_blend_attachment.blendEnable = VK_FALSE;
}

void Pipeline::set_blend(VkBlendFactor src, VkBlendFactor dst) {
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcColorBlendFactor = src;
	color_blend_attachment.dstColorBlendFactor = dst;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_MAX;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
}

void Pipeline::set_blend(float alpha) {
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	color_blending.blendConstants[0] = alpha;
	color_blending.blendConstants[1] = alpha;
	color_blending.blendConstants[2] = alpha;
	color_blending.blendConstants[3] = alpha;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_MAX;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
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

void Pipeline::set_dynamic(const Array<VkDynamicState> &_dynamic_states) {
	dynamic_states = _dynamic_states;
}

Pipeline* Pipeline::build(Shader *shader, RenderPass *render_pass, bool _create) {
	Pipeline *p = new Pipeline(shader, render_pass);
	pipelines.push_back(p);
	if (_create)
		p->create();
	return p;
}

void Pipeline::create() {
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapChainExtent.width;
	viewport.height = (float) swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	if (shader->push_size > 0) {
		VkPushConstantRange pci = {0};
		pci.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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

	VkPipelineDynamicStateCreateInfo dynami_state = {};
	dynami_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynami_state.dynamicStateCount = dynamic_states.num;
	dynami_state.pDynamicStates = &dynamic_states[0];

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = shader_stages.size();
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.layout = layout;
	pipeline_info.renderPass = render_pass->render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.pDynamicState = &dynami_state;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

void Pipeline::destroy() {
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, layout, nullptr);
}

};

#endif

