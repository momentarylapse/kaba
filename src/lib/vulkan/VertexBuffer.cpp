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
	vertex_count = 0;
	output_count = 0;
	index_type = VK_INDEX_TYPE_UINT16;

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

void VertexBuffer::build(const DynamicArray &array) {
	_create_buffer(vertex_buffer, array);
	vertex_count = array.num;
	output_count = array.num; // might be overwritten by creat_index_buffer()
}

void VertexBuffer::build_v3_v3_v2(const Array<Vertex1> &vertices) {
	build(vertices);
}

void VertexBuffer::build_v3_v3_v2_i(const Array<Vertex1> &vertices, const Array<int> &indices) {
	build_v3_v3_v2(vertices);
	_create_index_buffer_i32(indices);
}

void VertexBuffer::build_v3(const Array<vector> &vertices) {
	build(vertices);
}

void VertexBuffer::build_v3_i(const Array<vector> &vertices, const Array<int> &indices) {
	build_v3(vertices);
	_create_index_buffer_i32(indices);
}

/*void VertexBuffer::_create_index_buffer_i32(const Array<int> &indices) {
	Array<uint16_t> indices16;
	indices16.resize(indices.num);
	for (int i=0; i<indices.num; i++)
		indices16[i] = (unsigned)indices[i];
	for (auto i: indices16)
		std::cout << i << "  ";
	_create_index_buffer_i16(indices16);
}*/

void VertexBuffer::_create_buffer(Buffer &buf, const DynamicArray &array) {
	if (array.num == 0)
		return;

	VkDeviceSize buffer_size = array.num * array.element_size;

	// -> staging
	Buffer staging;
	staging.create(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	staging.update_part(array.data, 0, buffer_size);


	// gpu
	if (buffer_size > buf.size) {
		buf.destroy();
		//auto usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		auto usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		buf.create(buffer_size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	copy_buffer(staging.buffer, buf.buffer, buffer_size);
}

void VertexBuffer::_create_index_buffer_i16(const Array<uint16_t> &indices) {
	_create_buffer(index_buffer, indices);
	output_count = indices.num;
	index_type = VK_INDEX_TYPE_UINT16;
}

void VertexBuffer::_create_index_buffer_i32(const Array<int> &indices) {
	if (indices.num < (1<<16) and false) {
		Array<uint16_t> indices16;
		indices16.resize(indices.num);
		for (int i=0; i<indices.num; i++)
			indices16[i] = (unsigned)indices[i];
		_create_index_buffer_i16(indices16);
	} else {
		_create_buffer(index_buffer, indices);
		output_count = indices.num;
		index_type = VK_INDEX_TYPE_UINT32;
	}
}

};

#endif
