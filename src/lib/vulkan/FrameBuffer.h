/*
 * FrameBuffer.h
 *
 *  Created on: Jan 2, 2020
 *      Author: michi
 */

#pragma once

#if HAS_LIB_VULKAN

#include <vulkan/vulkan.h>
#include "../base/base.h"
#include "../base/pointer.h"
#include "Texture.h"

namespace vulkan {

class RenderPass;


class DepthBuffer : public Texture {
public:
	DepthBuffer(int w, int h, VkFormat format, bool with_sampler);
	DepthBuffer(int w, int h, const string &format, bool with_sampler);

	void create(int w, int h, VkFormat format);
};

class FrameBuffer : public Sharable<base::Empty> {
public:
	FrameBuffer(RenderPass *rp, const shared_array<Texture> &attachments);
	~FrameBuffer();

	VkFramebuffer frame_buffer;
	int width, height;

	void update(RenderPass *rp, const shared_array<Texture> &attachments);
	void update_x(RenderPass *rp, const shared_array<Texture> &attachments, int layer);

	void _create(RenderPass *rp, const shared_array<Texture> &attachments, int layer);
	void _destroy();

	shared_array<Texture> attachments;
	Array<VkImageView> cube_views;
};

}

#endif

