/*
 * FrameBuffer.cpp
 *
 *  Created on: Jan 2, 2020
 *      Author: michi
 */

#if HAS_LIB_VULKAN


#include "vulkan.h"
#include "FrameBuffer.h"
#include "helper.h"

namespace vulkan {

VkFormat parse_format(const string &s);


DepthBuffer::DepthBuffer(int w, int h, VkFormat _format, bool _with_sampler) {
	type = Type::DEPTH;
	create(w, h, _format);
}

DepthBuffer::DepthBuffer(int w, int h, const string &format, bool _with_sampler) : DepthBuffer(w, h, parse_format(format), _with_sampler) {}

void DepthBuffer::create(int w, int h, VkFormat format) {
	width = w;
	height = h;
	depth = 1;
	mip_levels = 1;

	auto usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	//if (!with_sampler)
	//	usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image.create(VK_IMAGE_TYPE_2D, width, height, 1, 1, 1, format, usage, false);
	view = image.create_view(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 0, 1);

	image.transition_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 0, 1);


	//if (with_sampler)
	_create_sampler();
}




FrameBuffer::FrameBuffer(RenderPass *rp, const shared_array<Texture> &attachments) {
	frame_buffer = nullptr;
	update(rp, attachments);
}

FrameBuffer::~FrameBuffer() {
	_destroy();
}



void FrameBuffer::update(RenderPass *rp, const shared_array<Texture> &_attachments) {
	update_x(rp, _attachments, 0);
}

void FrameBuffer::update_x(RenderPass *rp, const shared_array<Texture> &_attachments, int layer) {
	_destroy();
	_create(rp, _attachments, layer);
}

void FrameBuffer::_create(RenderPass *rp, const shared_array<Texture> &_attachments, int layer) {
	shared_array<Texture> new_attachments;
	for (auto a: weak(_attachments))
		new_attachments.add(a);
	attachments = new_attachments;

	width = 1;
	height = 1;
	if (attachments.num > 0) {
		width = attachments[0]->width;
		height = attachments[0]->height;
	}

	Array<VkImageView> views;
	for (auto a: weak(_attachments)) {
		if (a->type == Texture::Type::CUBE) {
			auto v = a->image.create_view(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, layer, 1);
			cube_views.add(v);
			views.add(v);
		} else {
			views.add(a->view);
		}
	}

	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = rp->render_pass;
	info.attachmentCount = views.num;
	info.pAttachments = &views[0];
	info.width = width;
	info.height = height;
	info.layers = 1;

	if (vkCreateFramebuffer(default_device->device, &info, nullptr, &frame_buffer) != VK_SUCCESS)
		throw Exception("failed to create framebuffer!");
}

void FrameBuffer::_destroy() {
	if (frame_buffer)
		vkDestroyFramebuffer(default_device->device, frame_buffer, nullptr);
	frame_buffer = nullptr;

	for (auto v: cube_views)
		vkDestroyImageView(default_device->device, v, nullptr);
	cube_views.clear();
}

rect FrameBuffer::area() const {
	return {0, (float)width, 0, (float)height};
}


} /* namespace vulkan */

#endif

