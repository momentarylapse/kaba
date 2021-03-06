#pragma once

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include <vulkan/vulkan.h>
#include <optional>

namespace vulkan{

	class FrameBuffer;

	//void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
	void create_image(uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);
	void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
	VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageViewType type, uint32_t mip_levels);
	void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);
	void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);

	uint32_t find_memory_type(const VkMemoryRequirements &requirements, VkMemoryPropertyFlags properties);
	bool has_stencil_component(VkFormat format);


	VkFormat find_supported_format(const Array<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat find_depth_format();

	extern VkDevice device;
	extern VkPhysicalDevice physical_device;

	extern VkSurfaceKHR surface;


	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		bool is_complete();
	};

	QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

};

#endif
