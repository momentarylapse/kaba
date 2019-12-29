#if HAS_LIB_VULKAN

#include "VertexBuffer.h"
#include <vulkan/vulkan.h>
#include <iostream>

#include "helper.h"

namespace vulkan{



VkVertexInputBindingDescription Vertex1::binding_description() {
	VkVertexInputBindingDescription bd = {};
	bd.binding = 0;
	bd.stride = sizeof(Vertex1);
	bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bd;
}

std::vector<VkVertexInputAttributeDescription> Vertex1::attribute_descriptions() {
	std::vector<VkVertexInputAttributeDescription> ad;
	ad.resize(3);

	ad[0].binding = 0;
	ad[0].location = 0;
	ad[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	ad[0].offset = offsetof(Vertex1, pos);

	ad[1].binding = 0;
	ad[1].location = 1;
	ad[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	ad[1].offset = offsetof(Vertex1, normal);

	ad[2].binding = 0;
	ad[2].location = 2;
	ad[2].format = VK_FORMAT_R32G32_SFLOAT;
	ad[2].offset = offsetof(Vertex1, u);

	return ad;
}



VertexBuffer::VertexBuffer() {
	vertex_buffer = nullptr;
	vertex_memory = nullptr;
	vertex_buffer_size = 0;
	index_buffer = nullptr;
	index_memory = nullptr;
	index_buffer_size = 0;
	output_count = 0;
}

VertexBuffer::~VertexBuffer() {
	if (index_buffer) {
		vkDestroyBuffer(device, index_buffer, nullptr);
		vkFreeMemory(device, index_memory, nullptr);
	}
	if (vertex_buffer) {
		vkDestroyBuffer(device, vertex_buffer, nullptr);
		vkFreeMemory(device, vertex_memory, nullptr);
	}
}

void VertexBuffer::__init__() {
	new(this) VertexBuffer();
}

void VertexBuffer::__delete__() {
	this->~VertexBuffer();
}

void VertexBuffer::build(const void *vertices, int size, int count) {
	_create_vertex_buffer(vertices, size * count);
	output_count = count;
}

void VertexBuffer::build1(const Array<Vertex1> &vertices) {
	build(vertices.data, sizeof(vertices[0]), vertices.num);
}

void VertexBuffer::build1i(const Array<Vertex1> &vertices, const Array<int> &indices) {
	build1(vertices);

	Array<uint16_t> indices16;
	indices16.resize(indices.num);
	for (int i=0; i<indices.num; i++)
		indices16[i] = indices[i];
	_create_index_buffer(indices16);
	output_count = indices.num;
}

void VertexBuffer::_create_vertex_buffer(const void *vdata, int size) {
	VkDeviceSize buffer_size = size;

	// -> staging
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_memory);

	void* data;
	vkMapMemory(device, staging_memory, 0, buffer_size, 0, &data);
		memcpy(data, vdata, (size_t) buffer_size);
	vkUnmapMemory(device, staging_memory);


	// gpu
	if (buffer_size > vertex_buffer_size) {
		if (vertex_buffer)
			vkDestroyBuffer(device, vertex_buffer, nullptr);
		if (vertex_memory)
			vkFreeMemory(device, vertex_memory, nullptr);

		create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_memory);
		vertex_buffer_size = buffer_size;
	}

	copy_buffer(staging_buffer, vertex_buffer, buffer_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_memory, nullptr);
}

void VertexBuffer::_create_index_buffer(const Array<uint16_t> &indices) {
	VkDeviceSize buffer_size = sizeof(indices[0]) * indices.num;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_memory);

	void* data;
	vkMapMemory(device, staging_memory, 0, buffer_size, 0, &data);
		memcpy(data, indices.data, (size_t) buffer_size);
	vkUnmapMemory(device, staging_memory);

	if (buffer_size > index_buffer_size) {
		if (index_buffer)
			vkDestroyBuffer(device, index_buffer, nullptr);
		if (index_memory)
			vkFreeMemory(device, index_memory, nullptr);

		create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_memory);
		index_buffer_size = buffer_size;
	}

	copy_buffer(staging_buffer, index_buffer, buffer_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_memory, nullptr);
}

};

#endif
