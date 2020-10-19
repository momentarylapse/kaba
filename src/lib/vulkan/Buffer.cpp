/*
 * Buffer.cpp
 *
 *  Created on: 19.10.2020
 *      Author: michi
 */

#if HAS_LIB_VULKAN


#include "Buffer.h"
#include <vulkan/vulkan.h>

#include <array>
#include <iostream>

#include "helper.h"


namespace vulkan{

Array<UniformBuffer*> ubo_wrappers;

extern VkPhysicalDeviceProperties device_properties;
int make_aligned(int size) {
	if (device_properties.limits.minUniformBufferOffsetAlignment == 0)
		return 0;
	return (size + device_properties.limits.minUniformBufferOffsetAlignment - 1) & ~(size - 1);
}



Buffer::Buffer() {
	buffer = nullptr;
	memory = nullptr;
	size = 0;
}

Buffer::~Buffer() {
	destroy();
}

void Buffer::create(VkDeviceSize _size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
	size = _size;
	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = size;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &info, nullptr, &buffer) != VK_SUCCESS) {
		throw Exception("failed to create buffer!");
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = find_memory_type(mem_requirements, properties);

	if (vkAllocateMemory(device, &alloc_info, nullptr, &memory) != VK_SUCCESS) {
		throw Exception("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, memory, 0);
}

void Buffer::destroy() {
	if (buffer)
		vkDestroyBuffer(device, buffer, nullptr);
	buffer = nullptr;
	if (memory)
		vkFreeMemory(device, memory, nullptr);
	memory = nullptr;
	size = 0;
}

void *Buffer::map(VkDeviceSize _offset, VkDeviceSize _size) {
	void *p;
	vkMapMemory(device, memory, _offset, _size, 0, &p);
	return p;
}

void Buffer::unmap() {
	vkUnmapMemory(device, memory);
}

void Buffer::update_part(const void *source, int offset, int update_size) {
	void* data = map(offset, update_size);
	memcpy(data, source, update_size);
	unmap();
}

void Buffer::update(void *source) {
	update_part(source, 0, size);
}



UniformBuffer::UniformBuffer(int _size) {
	count = 0;
	size = _size;
	size_single = size;
	size_single_aligned = size;
	VkDeviceSize buffer_size = size;

	create(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

UniformBuffer::UniformBuffer(int _size, int _count) {
	// "dynamic"
	count = _count;
	size_single = _size;
	size_single_aligned = make_aligned(size_single);
	size = size_single_aligned * count;
	VkDeviceSize buffer_size = size;

	create(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

UniformBuffer::~UniformBuffer() {
}
void UniformBuffer::__init__(int size) {
	new(this) UniformBuffer(size);
}
void UniformBuffer::__delete__() {
	this->~UniformBuffer();
}

bool UniformBuffer::is_dynamic() {
	return count > 0;
}

void UniformBuffer::update_single(void *source, int index) {
	update_part(source, size_single_aligned * index, size_single);
}





}

#endif
