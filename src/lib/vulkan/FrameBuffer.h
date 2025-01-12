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

class rect;

namespace vulkan {

class RenderPass;


class DepthBuffer : public Texture {
public:
	DepthBuffer(int w, int h, VkFormat format, bool with_sampler = true);
	DepthBuffer(int w, int h, const string &format, bool with_sampler = true);

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

	rect area() const;

	shared_array<Texture> attachments;
	Array<VkImageView> cube_views;
};

}

#endif

