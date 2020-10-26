/*
 * Buffer.h
 *
 *  Created on: 19.10.2020
 *      Author: michi
 */
#pragma once

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include <vulkan/vulkan.h>

namespace vulkan {

	class Buffer {
	public:
		Buffer();
		~Buffer();
		void create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		void destroy();
		void *map();
		void *map_part(VkDeviceSize offset, VkDeviceSize size);
		void unmap();
		void update(void *source);
		void update_part(const void *source, int offset, int update_size);
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDeviceSize size;
	};

	class UniformBuffer : public Buffer {
	public:
		UniformBuffer(int size);
		UniformBuffer(int size, int count);
		~UniformBuffer();

		void __init__(int size);
		void __init_multi__(int size, int count);
		void __delete__();

		void update_single(void *source, int index);

		bool is_dynamic();

		int size_single;
		int count, size_single_aligned;
	};

}

#endif
