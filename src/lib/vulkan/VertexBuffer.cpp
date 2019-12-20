#if HAS_LIB_VULKAN

#include "VertexBuffer.h"
#include <vulkan/vulkan.h>

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
	index_buffer = nullptr;
	index_memory = nullptr;
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

VertexBuffer* VertexBuffer::build1(const Array<Vertex1> &vertices) {
	VertexBuffer *vb = new VertexBuffer();
	vb->_create_vertex_buffer(vertices.data, sizeof(vertices[0]) * vertices.num);
	vb->output_count = vertices.num;
	return vb;
}

VertexBuffer* VertexBuffer::build1i(const Array<Vertex1> &vertices, const Array<uint16_t> &indices) {
	VertexBuffer *vb = build1(vertices);
	vb->_create_index_buffer(indices);
	vb->output_count = indices.num;
	return vb;
}

void VertexBuffer::_create_vertex_buffer(void *vdata, int size) {
	VkDeviceSize buffer_size = size;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_memory);

	void* data;
	vkMapMemory(device, staging_memory, 0, buffer_size, 0, &data);
		memcpy(data, vdata, (size_t) buffer_size);
	vkUnmapMemory(device, staging_memory);

	createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_memory);

	copyBuffer(staging_buffer, vertex_buffer, buffer_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_memory, nullptr);
}

void VertexBuffer::_create_index_buffer(const Array<uint16_t> &indices) {
	VkDeviceSize buffer_size = sizeof(indices[0]) * indices.num;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_memory);

	void* data;
	vkMapMemory(device, staging_memory, 0, buffer_size, 0, &data);
		memcpy(data, indices.data, (size_t) buffer_size);
	vkUnmapMemory(device, staging_memory);

	createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_memory);

	copyBuffer(staging_buffer, index_buffer, buffer_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_memory, nullptr);
}

};

#endif
