#pragma once

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include <vulkan/vulkan.h>

namespace vulkan{

	class FrameBuffer;

	struct ImageAndMemory {
		VkImage image = nullptr;
		VkDeviceMemory memory = nullptr;
		VkFormat format = VK_FORMAT_UNDEFINED;

		void create(VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t num_layers, VkFormat format, VkImageUsageFlags usage, bool cube);
		void _destroy();


		void generate_mipmaps(uint32_t width, uint32_t height, uint32_t mip_levels, uint32_t layer0, uint32_t num_layers, VkImageLayout new_layout);

		VkImageView create_view(VkImageAspectFlags aspect_flags, VkImageViewType type, uint32_t mip_levels, uint32_t layer0, uint32_t num_layers) const;
		void transition_layout(VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels, uint32_t layer0, uint32_t num_layers) const;


		bool is_depth_buffer() const;
		bool has_stencil_component() const;
	};

	//void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
	void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
	void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth, uint32_t level, uint32_t layer);
};

#endif
