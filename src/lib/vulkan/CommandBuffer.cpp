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
#include "VertexBuffer.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Shader.h"
#include "../image/color.h"
#include "../math/rect.h"
#include <array>

#include <iostream>

namespace vulkan{

	VkCommandPool command_pool;
	extern Array<VkFramebuffer> swap_chain_framebuffers;
	extern VkQueue graphics_queue;
	extern VkQueue present_queue;
	uint32_t image_index;
	extern int target_width, target_height;


void create_command_pool() {
	QueueFamilyIndices queue_family_indices = find_queue_families(physical_device);

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

	if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
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

	VkCommandBufferBeginInfo bi = {};
	bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &bi);

	return command_buffer;
}

void end_single_time_commands(VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo si = {};
	si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	si.commandBufferCount = 1;
	si.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &si, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

CommandBuffer::CommandBuffer() {
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
	buffers.resize(swap_chain_framebuffers.num);

	VkCommandBufferAllocateInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.commandPool = command_pool;
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ai.commandBufferCount = (uint32_t)buffers.num;

	if (vkAllocateCommandBuffers(device, &ai, &buffers[0]) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void CommandBuffer::_destroy() {
	vkFreeCommandBuffers(device, command_pool, buffers.num, &buffers[0]);
}


//VkDescriptorSet current_set;

void CommandBuffer::set_pipeline(Pipeline *pl) {
	vkCmdBindPipeline(current, VK_PIPELINE_BIND_POINT_GRAPHICS, pl->pipeline);
	current_pipeline = pl;
}
void CommandBuffer::bind_descriptor_set(int index, DescriptorSet *dset) {
	vkCmdBindDescriptorSets(current, VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline->layout, index, 1, &dset->descriptor_sets[image_index], 0, nullptr);
}
void CommandBuffer::push_constant(int offset, int size, void *data) {
	auto stage_flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	vkCmdPushConstants(current, current_pipeline->layout, stage_flags, offset, size, data);
}

void CommandBuffer::draw(VertexBuffer *vb) {
	VkBuffer vertexBuffers[] = {vb->vertex_buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(current, 0, 1, vertexBuffers, offsets);

	if (vb->index_buffer) {
		vkCmdBindIndexBuffer(current, vb->index_buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(current, vb->output_count, 1, 0, 0, 0);
	} else {
		vkCmdDraw(current, vb->output_count, 1, 0, 0);
	}
}

void CommandBuffer::begin() {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	current = buffers[image_index];
	if (vkBeginCommandBuffer(current, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
}

void CommandBuffer::begin_render_pass(RenderPass *rp, const color &clear_color) {
	std::array<VkClearValue, 2> cv = {};
	memcpy((void*)&cv[0].color, &clear_color, sizeof(color));
	cv[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo rpi = {};
	rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpi.renderPass = rp->render_pass;
	rpi.framebuffer = swap_chain_framebuffers[image_index];
	rpi.renderArea.offset = {0, 0};
	rpi.renderArea.extent = swap_chain_extent;
	rpi.clearValueCount = static_cast<uint32_t>(cv.size());
	rpi.pClearValues = cv.data();

	vkCmdBeginRenderPass(current, &rpi, VK_SUBPASS_CONTENTS_INLINE);

	VkRect2D scissor = {0, 0, (unsigned)target_width, (unsigned)target_height};
	vkCmdSetScissor(current, 0, 1, &scissor);
}

void CommandBuffer::scissor(const rect &r) {
	VkRect2D scissor = {(int)r.x1, (int)r.y1, (unsigned int)r.width(), (unsigned int)r.height()};
	vkCmdSetScissor(current, 0, 1, &scissor);
}


void CommandBuffer::end_render_pass() {
	vkCmdEndRenderPass(current);
}

void CommandBuffer::end() {
	if (vkEndCommandBuffer(current) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}


};

#endif
