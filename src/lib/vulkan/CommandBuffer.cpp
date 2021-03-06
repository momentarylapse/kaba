//
//  CommandBuffer.cpp
//  3
//
//  Created by <author> on 06/02/2019.
//
//

#if HAS_LIB_VULKAN


#include "CommandBuffer.h"
#include "helper.h"
#include "DescriptorSet.h"
#include "VertexBuffer.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Shader.h"
#include "FrameBuffer.h"
#include "vulkan.h"
#include "../image/color.h"
#include "../math/rect.h"
#include <array>

#include <iostream>

namespace vulkan{

	VkCommandPool command_pool;
	extern VkQueue graphics_queue;


void create_command_pool() {
	QueueFamilyIndices queue_family_indices = find_queue_families(physical_device);

	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.queueFamilyIndex = queue_family_indices.graphics_family.value();
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &info, nullptr, &command_pool) != VK_SUCCESS) {
		throw Exception("failed to create command pool!");
	}
}

void destroy_command_pool() {
	vkDestroyCommandPool(device, command_pool, nullptr);
}


VkCommandBuffer begin_single_time_commands() {
	VkCommandBufferAllocateInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ai.commandPool = command_pool;
	ai.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &ai, &command_buffer);

	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &info);

	return command_buffer;
}

void end_single_time_commands(VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

CommandBuffer::CommandBuffer() {
	cur_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
	_create();
}

CommandBuffer::~CommandBuffer() {
	_destroy();
}

void CommandBuffer::__init__() {
	new(this) CommandBuffer();
}

void CommandBuffer::__delete__() {
	this->~CommandBuffer();
}

void CommandBuffer::_create() {

	VkCommandBufferAllocateInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.commandPool = command_pool;
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ai.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &ai, &buffer) != VK_SUCCESS) {
		throw Exception("failed to allocate command buffers!");
	}
}

void CommandBuffer::_destroy() {
	vkFreeCommandBuffers(device, command_pool, 1, &buffer);
}


//VkDescriptorSet current_set;

void CommandBuffer::set_bind_point(const string &s) {
	if (s == "graphics")
		cur_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
	else if (s == "raytracing")
		cur_bind_point = VK_PIPELINE_BIND_POINT_RAY_TRACING_NV;
	else if (s == "compute")
		cur_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
	else
		throw Exception("unknown bind point: " + s);
}

void CommandBuffer::bind_pipeline(BasePipeline *pl) {
	vkCmdBindPipeline(buffer, cur_bind_point, pl->pipeline);
	current_pipeline = pl;
}

void CommandBuffer::bind_descriptor_set(int index, DescriptorSet *dset) {
	if (dset->num_dynamic_ubos > 0)
		throw Exception("descriptor set requires dynamic indices");
	vkCmdBindDescriptorSets(buffer, cur_bind_point, current_pipeline->layout, index, 1, &dset->descriptor_set, 0, nullptr);
}

void CommandBuffer::bind_descriptor_set_dynamic(int index, DescriptorSet *dset, const Array<int> &indices) {
	if (dset->num_dynamic_ubos != indices.num)
		throw Exception("number of indices does not match descriptor set");
	Array<unsigned int> offsets;
	int i = 0;
	for (auto &b: dset->buffers) {
		if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
			offsets.add(b.info.range * indices[i]);
			i ++;
		}
	}
	vkCmdBindDescriptorSets(buffer, cur_bind_point, current_pipeline->layout, index, 1, &dset->descriptor_set, offsets.num, (unsigned*)&offsets[0]);
}

void CommandBuffer::push_constant(int offset, int size, void *data) {
	auto stage_flags = VK_SHADER_STAGE_VERTEX_BIT /*| VK_SHADER_STAGE_GEOMETRY_BIT*/ | VK_SHADER_STAGE_FRAGMENT_BIT;
	vkCmdPushConstants(buffer, current_pipeline->layout, stage_flags, offset, size, data);
}

void CommandBuffer::draw(VertexBuffer *vb) {
	if (vb->output_count == 0)
		return;
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(buffer, 0, 1, &vb->vertex_buffer.buffer, offsets);

	if (vb->index_buffer.buffer) {
		vkCmdBindIndexBuffer(buffer, vb->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(buffer, vb->output_count, 1, 0, 0, 0);
	} else {
		vkCmdDraw(buffer, vb->output_count, 1, 0, 0);
	}
}

void CommandBuffer::begin() {
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (vkBeginCommandBuffer(buffer, &info) != VK_SUCCESS) {
		throw Exception("failed to begin recording command buffer!");
	}
}

void CommandBuffer::begin_render_pass(RenderPass *rp, FrameBuffer *fb) {
	Array<VkClearValue> clear_values;
	for (auto &c: rp->clear_color) {
		VkClearValue cv = {};
		memcpy((void*)&cv.color, &c, sizeof(color));
		clear_values.add(cv);
	}
	VkClearValue cv = {};
	cv.depthStencil = {rp->clear_z, rp->clear_stencil};
	clear_values.add(cv);

	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = rp->render_pass;
	info.framebuffer = fb->frame_buffer;
	info.renderArea.offset = {0, 0};
	info.renderArea.extent = {(unsigned)fb->width, (unsigned)fb->height};
	info.clearValueCount = clear_values.num;
	info.pClearValues = &clear_values[0];

	vkCmdBeginRenderPass(buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
}
void CommandBuffer::next_subpass() {
	vkCmdNextSubpass(buffer, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::set_scissor(const rect &r) {
	VkRect2D scissor = {(int)r.x1, (int)r.y1, (unsigned int)r.width(), (unsigned int)r.height()};
	vkCmdSetScissor(buffer, 0, 1, &scissor);
}

void CommandBuffer::set_viewport(const rect &r) {
	VkViewport viewport = {r.x1, r.y1, r.width(), r.height(), 0.0f, 1.0f};
	vkCmdSetViewport(buffer, 0, 1, &viewport);
}

void CommandBuffer::dispatch(int nx, int ny, int nz) {
	vkCmdDispatch(buffer, nx, ny, nz);
}

void CommandBuffer::trace_rays(int nx, int ny, int nz) {
	auto rtp = static_cast<RayPipeline*>(current_pipeline);
	int hs = rtx::properties.shaderGroupHandleSize;
	_vkCmdTraceRaysNV(buffer,
			rtp->sbt.buffer, 0,
			rtp->sbt.buffer, rtp->miss_group_offset*hs, hs,
			rtp->sbt.buffer, hs, hs,
			VK_NULL_HANDLE, 0, 0,
			nx, ny, nz);
}

bool image_is_depth_buffer(VkFormat f) {
	return (f == VK_FORMAT_D32_SFLOAT) or (f == VK_FORMAT_D32_SFLOAT_S8_UINT) or (f == VK_FORMAT_D24_UNORM_S8_UINT) or (f == VK_FORMAT_D16_UNORM);
}

void CommandBuffer::barrier(const Array<Texture*> &textures, int mode) {
	Array<VkImageMemoryBarrier> barriers;
	for (auto *t: textures) {
		bool is_depth = image_is_depth_buffer(t->format);
		VkImageSubresourceRange sr = {};
		sr.aspectMask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		sr.baseArrayLayer = 0;
		sr.baseMipLevel = 0;
		sr.layerCount = 1;
		sr.levelCount = 1;
		VkImageMemoryBarrier b = {};
		b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		b.srcAccessMask = is_depth ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		b.oldLayout = is_depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		b.image = t->image;
		b.subresourceRange = sr;
		barriers.add(b);

	}

	auto source_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	auto destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	vkCmdPipelineBarrier(
		buffer,
		source_stage, destination_stage,
		0,
		0, nullptr,
		0, nullptr,
		barriers.num, &barriers[0]
	);
}


void CommandBuffer::image_barrier(const Texture *t, const Array<int> &flags) {

	bool is_depth = image_is_depth_buffer(t->format);
	VkImageSubresourceRange sr = {};
	sr.aspectMask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	sr.baseArrayLayer = 0;
	sr.baseMipLevel = 0;
	sr.layerCount = 1;
	sr.levelCount = 1;

	VkImageMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = (VkAccessFlags)flags[0];
	barrier.dstAccessMask = (VkAccessFlags)flags[1];
	barrier.oldLayout = (VkImageLayout)flags[0];
	barrier.newLayout = (VkImageLayout)flags[0];
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = t->image;
	barrier.subresourceRange = sr;

	vkCmdPipelineBarrier(buffer,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0, 0, nullptr, 0, nullptr, 1,
			&barrier);
}

void CommandBuffer::copy_image(const Texture *source, const Texture *dest, const Array<int> &extend) {

	bool src_is_depth = image_is_depth_buffer(source->format);
	bool dst_is_depth = image_is_depth_buffer(dest->format);

	VkImageCopy region;
	region.srcSubresource = {src_is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
	region.srcOffset = {extend[0], extend[1], 0};
	region.dstSubresource = {dst_is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
	region.dstOffset = {extend[4], extend[5], 0};
	region.extent = {(unsigned)extend[2], (unsigned)extend[3], 1};
	vkCmdCopyImage(buffer,
			source->image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dest->image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
}


void CommandBuffer::end_render_pass() {
	vkCmdEndRenderPass(buffer);
}

void CommandBuffer::end() {
	if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
		throw Exception("failed to record command buffer!");
	}
}


};

#endif
