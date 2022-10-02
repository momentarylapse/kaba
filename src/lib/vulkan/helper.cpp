#if HAS_LIB_VULKAN

#include <vulkan/vulkan.h>
#include "helper.h"
#include "CommandBuffer.h"
#include "Device.h"
#include <vector>

namespace vulkan{



void ImageAndMemory::_destroy() {
	if (image)
		vkDestroyImage(default_device->device, image, nullptr);
	if (memory)
		vkFreeMemory(default_device->device, memory, nullptr);
	image = nullptr;
	memory = nullptr;
}

void ImageAndMemory::create(VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t num_layers, VkFormat _format, VkImageUsageFlags usage, bool cube) {
	format = _format;

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = type;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = depth;
	image_info.mipLevels = mip_levels;
	image_info.arrayLayers = num_layers;
	image_info.format = format;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (cube)
		image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	if (vkCreateImage(default_device->device, &image_info, nullptr, &image) != VK_SUCCESS) {
		throw Exception("failed to create image!");
	}

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(default_device->device, image, &mem_requirements);

	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = default_device->find_memory_type(mem_requirements, properties);

	if (vkAllocateMemory(default_device->device, &alloc_info, nullptr, &memory) != VK_SUCCESS) {
		throw Exception("failed to allocate image memory!");
	}

	vkBindImageMemory(default_device->device, image, memory, 0);
}

void ImageAndMemory::generate_mipmaps(uint32_t width, uint32_t height, uint32_t mip_levels, uint32_t layer0, uint32_t num_layers, VkImageLayout new_layout) {
	// Check if image format supports linear blitting
	VkFormatProperties fp;
	vkGetPhysicalDeviceFormatProperties(default_device->physical_device, format, &fp);

	if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw Exception("texture image format does not support linear blitting!");
	}

	auto command_buffer = begin_single_time_commands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = layer0;
	barrier.subresourceRange.layerCount = num_layers;
	barrier.subresourceRange.levelCount = 1;

	int32_t mip_width = width;
	int32_t mip_height = height;

	for (int i=1; i<mip_levels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(command_buffer->buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {mip_width, mip_height, 1};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = layer0;
		blit.srcSubresource.layerCount = num_layers;
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = layer0;
		blit.dstSubresource.layerCount = num_layers;

		vkCmdBlitImage(command_buffer->buffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(command_buffer->buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mip_width > 1) mip_width /= 2;
		if (mip_height > 1) mip_height /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mip_levels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = new_layout;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(command_buffer->buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	end_single_time_commands(command_buffer);
}

bool format_is_depth_buffer(VkFormat f) {
	return (f == VK_FORMAT_D32_SFLOAT) or (f == VK_FORMAT_D32_SFLOAT_S8_UINT) or (f == VK_FORMAT_D24_UNORM_S8_UINT) or (f == VK_FORMAT_D16_UNORM);
}

bool ImageAndMemory::is_depth_buffer() const {
	return format_is_depth_buffer(format);
}
bool ImageAndMemory::has_stencil_component() const {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT or format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkImageAspectFlagBits ImageAndMemory::aspect() const {
	if (is_depth_buffer())
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	return VK_IMAGE_ASPECT_COLOR_BIT;
}

void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
	auto command_buffer = begin_single_time_commands();

	VkBufferCopy copy_region = {};
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer->buffer, src_buffer, dst_buffer, 1, &copy_region);

	end_single_time_commands(command_buffer);
}

VkImageView ImageAndMemory::create_view(VkImageAspectFlags aspect, VkImageViewType type, uint32_t mip_levels, uint32_t layer0, uint32_t num_layers) const {
	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.image = image;
	info.viewType = type;
	info.format = format;
	info.subresourceRange.aspectMask = aspect;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = mip_levels;
	info.subresourceRange.baseArrayLayer = layer0;
	info.subresourceRange.layerCount = num_layers;

	VkImageView image_view;
	if (vkCreateImageView(default_device->device, &info, nullptr, &image_view) != VK_SUCCESS)
		throw Exception("failed to create texture image view!");

	return image_view;
}

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth, uint32_t level, uint32_t layer) {
	auto command_buffer = begin_single_time_commands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = level;
	region.imageSubresource.baseArrayLayer = layer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, depth};

	vkCmdCopyBufferToImage(command_buffer->buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	end_single_time_commands(command_buffer);
}

VkImageAspectFlags image_aspect(const ImageAndMemory &i, VkImageLayout new_layout) {
	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		VkImageAspectFlags a = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (i.has_stencil_component())
			a |= VK_IMAGE_ASPECT_STENCIL_BIT;
		return a;
	} else {
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

void ImageAndMemory::transition_layout(VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels, uint32_t layer0, uint32_t num_layers) const {
	auto command_buffer = begin_single_time_commands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.aspectMask = image_aspect(*this, new_layout);
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mip_levels;
	barrier.subresourceRange.baseArrayLayer = layer0;
	barrier.subresourceRange.layerCount = num_layers;

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL and new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED and new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL and new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED and new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		throw Exception("unsupported layout transition!");
	}

	//command_buffer->image_barrier();

	vkCmdPipelineBarrier(
		command_buffer->buffer,
		source_stage, destination_stage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	end_single_time_commands(command_buffer);
}








};

#endif

