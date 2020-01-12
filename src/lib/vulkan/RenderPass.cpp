//
//  RenderPass.cpp
//  3
//
//  Created by <author> on 06/02/2019.
//
//

#include "RenderPass.h"
#include "helper.h"

#include <iostream>

#if HAS_LIB_VULKAN


Array<int> _range(int num) {
	Array<int> r;
	for (int i=0; i<num; i++)
		r.add(i);
	return r;
}

VkPipelineStageFlagBits parse_pipeline_stage(const string &s) {
	if (s == "TOP_OF_PIPE")
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	if (s == "DRAW_INDIRECT")
		return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
	if (s == "VERTEX_INPUT")
		return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	if (s == "VERTEX_SHADER")
		return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	if (s == "TESSELLATION_CONTROL_SHADER")
		return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
	if (s == "TESSELLATION_EVALUATION_SHADER")
		return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
	if (s == "GEOMETRY_SHADER")
		return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
	if (s == "FRAGMENT_SHADER")
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	if (s == "EARLY_FRAGMENT_TESTS")
		return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	if (s == "LATE_FRAGMENT_TESTS")
		return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	if (s == "COLOR_ATTACHMENT_OUTPUT")
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	if (s == "COMPUTE_SHADER")
		return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	if (s == "TRANSFER")
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	if (s == "BOTTOM_OF_PIPE")
		return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	if (s == "HOST")
		return VK_PIPELINE_STAGE_HOST_BIT;
	if (s == "ALL_GRAPHICS")
		return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	if (s == "ALL_COMMANDS")
		return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	std::cerr << " UNHANDLED PIPELINE STAGE: " << s.c_str() << "\n";
	return (VkPipelineStageFlagBits)0;
}

VkAccessFlags parse_access(const string &s) {
	if (s == "INDIRECT_COMMAND_READ")
		return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
	if (s == "INDEX_READ")
		return VK_ACCESS_INDEX_READ_BIT;
	if (s == "VERTEX_ATTRIBUTE_READ")
		return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	if (s == "UNIFORM_READ")
		return VK_ACCESS_UNIFORM_READ_BIT;
	if (s == "INPUT_ATTACHMENT_READ")
		return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	if (s == "SHADER_READ")
		return VK_ACCESS_SHADER_READ_BIT;
	if (s == "SHADER_WRITE")
		return VK_ACCESS_SHADER_WRITE_BIT;
	if (s == "COLOR_ATTACHMENT_READ")
		return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	if (s == "COLOR_ATTACHMENT_WRITE")
		return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	if (s == "DEPTH_STENCIL_ATTACHMENT_READ")
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	if (s == "DEPTH_STENCIL_ATTACHMENT_WRITE")
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	if (s == "TRANSFER_READ")
		return VK_ACCESS_TRANSFER_READ_BIT;
	if (s == "TRANSFER_WRITE")
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	if (s == "HOST_READ")
		return VK_ACCESS_HOST_READ_BIT;
	if (s == "HOST_WRITE")
		return VK_ACCESS_HOST_WRITE_BIT;
	if (s == "MEMORY_READ")
		return VK_ACCESS_MEMORY_READ_BIT;
	if (s == "MEMORY_WRITE")
		return VK_ACCESS_MEMORY_WRITE_BIT;
	std::cerr << " UNHANDLED ACCESS FLAGS: " << s.c_str() << "\n";
	return (VkAccessFlags)0;
}


namespace vulkan {

bool image_is_depth_buffer(VkFormat f);

// so far, we can only create a "default" render pass with 1 color and 1 depth attachement!
	RenderPass::RenderPass(const Array<VkFormat> &format, const string &options) {
		render_pass = nullptr;
		auto_subpasses = false;
		auto_dependencies = false;

		VkAttachmentLoadOp color_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
		VkAttachmentLoadOp depth_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
		if (options.find("clear") >= 0) {
			color_load_op = depth_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}

		// attachments
		for (auto &f: format) {
			VkAttachmentDescription a = {};
			a.format = f;
			if (image_is_depth_buffer(f)) {
				// depth buffer
				a.samples = VK_SAMPLE_COUNT_1_BIT;
				a.loadOp = depth_load_op;
				a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;//VK_ATTACHMENT_STORE_OP_DONT_CARE; // ARGH, might be needed for shadow maps
				a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				a.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			} else {
				// color attachment
				a.samples = VK_SAMPLE_COUNT_1_BIT;
				a.loadOp = color_load_op;
				a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				if (options.find("present") >= 0)
					a.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				else
					//a.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					a.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			attachments.add(a);
		}


		// auto subpass
		add_subpass(_range(format.num - 1), format.num - 1);
		auto_subpasses = true;


		// auto dependencies
		_add_dependency(-1,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				0,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		_add_dependency(0,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				-1,
				VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,//VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		auto_dependencies = true;

		for (int i=0; i<format.num - 1; i++)
			clear_color.add(Black);
		clear_z = 1.0f;
		clear_stencil = 0;

		create();
	}

	RenderPass::~RenderPass() {
		destroy();
	}


	void RenderPass::__init__(const Array<VkFormat> &format, const string &options) {
		new(this) RenderPass(format, options);
	}

	void RenderPass::__delete__() {
		this->~RenderPass();
	}

	void RenderPass::create() {

		subpasses.clear();
		for (auto &sd: subpass_data) {
			VkSubpassDescription sub = {};
			sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			sub.colorAttachmentCount = sd.color_attachment_refs.num;
			sub.pColorAttachments = &sd.color_attachment_refs[0];
			sub.pDepthStencilAttachment = &sd.depth_attachment_ref;
			subpasses.add(sub);
		}


		VkRenderPassCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = attachments.num;
		info.pAttachments = &attachments[0];
		info.subpassCount = subpasses.num;
		info.pSubpasses = &subpasses[0];
		info.dependencyCount = dependencies.num;
		info.pDependencies = &dependencies[0];

		if (vkCreateRenderPass(device, &info, nullptr, &render_pass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void RenderPass::destroy() {
		if (render_pass)
			vkDestroyRenderPass(device, render_pass, nullptr);
		render_pass = nullptr;
	}

	void RenderPass::rebuild() {
		destroy();
		create();
	}

	void RenderPass::add_subpass(const Array<int> &color_att, int depth_att) {
		if (auto_subpasses)
			subpass_data.clear();

		SubpassData sd;
		for (int i: color_att) {
			VkAttachmentReference r = {};
			r.attachment = i;
			r.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			sd.color_attachment_refs.add(r);
		}
		sd.depth_attachment_ref = {};
		sd.depth_attachment_ref.attachment = depth_att;
		sd.depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		subpass_data.add(sd);
	}

	void RenderPass::_add_dependency(int src, VkPipelineStageFlagBits src_stage, VkAccessFlags src_access, int dst, VkPipelineStageFlagBits dst_stage, VkAccessFlags dst_access) {
		if (auto_dependencies)
			dependencies.clear();

		VkSubpassDependency dep = {};
		dep.srcSubpass = (src < 0) ? VK_SUBPASS_EXTERNAL : src;
		dep.srcStageMask = src_stage;
		dep.srcAccessMask = src_access;
		dep.dstSubpass = (dst < 0) ? VK_SUBPASS_EXTERNAL : dst;
		dep.dstStageMask = dst_stage;
		dep.dstAccessMask = dst_access;
		dependencies.add(dep);
	}

	void _parse_dep_opt(const string &o, VkPipelineStageFlagBits &stage, VkAccessFlags &access) {
		auto oo = o.replace(" ", "").replace("\n", "").replace("\t", "").replace("-", "_").upper().explode(":");
		if (!oo.num == 2)
			throw std::runtime_error("opt-string needs one ':'");

		auto ss = oo[0].explode("|");
		stage = (VkPipelineStageFlagBits)0;
		for (auto &s: ss)
			stage = (VkPipelineStageFlagBits)(stage | parse_pipeline_stage(s));

		ss = oo[1].explode("|");
		access = (VkAccessFlags)0;
		for (auto &s: ss)
			access |= parse_access(s);
	}


	void RenderPass::add_dependency(int src, const string &src_opt, int dst, const string &dst_opt) {
		VkPipelineStageFlagBits src_stage, dst_stage;
		VkAccessFlags src_access, dst_access;
		_parse_dep_opt(src_opt, src_stage, src_access);
		_parse_dep_opt(dst_opt, dst_stage, dst_access);
		_add_dependency(src, src_stage, src_access, dst, dst_stage, dst_access);


	}

};

#endif
