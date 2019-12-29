#ifndef _NIX_HELPER_H
#define _NIX_HELPER_H

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include <vulkan/vulkan.h>
#include <optional>

namespace vulkan{

	void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void create_image(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);
	void transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	bool has_stencil_component(VkFormat format);


	VkFormat find_supported_format(const Array<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat find_depth_format();

	extern VkDevice device;
	extern VkPhysicalDevice physical_device;

	extern VkSurfaceKHR surface;

	extern Array<VkImage> swap_chain_images;
	extern VkExtent2D swap_chain_extent;


	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		bool is_complete();
	};

	QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

};

#endif

#endif
