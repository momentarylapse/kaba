#if HAS_LIB_VULKAN

#include "VertexBuffer.h"
#include <vulkan/vulkan.h>
#include <iostream>

#include "helper.h"

namespace vulkan{

Array<VertexBuffer*> vertex_buffers;

Vertex1::Vertex1(const vector &p, const vector &n, float _u, float _v) {
	pos = p;
	normal = n;
	u = _u;
	v = _v;
}

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
	output_count = 0;

	vertex_buffers.add(this);
}

VertexBuffer::~VertexBuffer() {
	_destroy();

	for (int i=0; i<vertex_buffers.num; i++)
		if (vertex_buffers[i] == this)
			vertex_buffers.erase(i);
}

void VertexBuffer::__init__() {
	new(this) VertexBuffer();
}

void VertexBuffer::__delete__() {
	this->~VertexBuffer();
}

void VertexBuffer::_destroy() {
	index_buffer.destroy();
	vertex_buffer.destroy();
	output_count = 0;
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
	if (size == 0)
		return;

	VkDeviceSize buffer_size = size;

	// -> staging
	Buffer staging;
	staging.create(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	staging.update_part(vdata, 0, buffer_size);


	// gpu
	if (buffer_size > vertex_buffer.size) {
		vertex_buffer.destroy();
		vertex_buffer.create(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	copy_buffer(staging.buffer, vertex_buffer.buffer, buffer_size);
}

void VertexBuffer::_create_index_buffer(const Array<uint16_t> &indices) {
	if (indices.num == 0)
		return;

	VkDeviceSize buffer_size = sizeof(indices[0]) * indices.num;

	Buffer staging;
	staging.create(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	staging.update_part(indices.data, 0, buffer_size);

	if (buffer_size > index_buffer.size) {
		index_buffer.destroy();
		index_buffer.create(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	copy_buffer(staging.buffer, index_buffer.buffer, buffer_size);
}

};

#endif
