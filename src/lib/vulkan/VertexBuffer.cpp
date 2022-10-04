#if HAS_LIB_VULKAN

#include "VertexBuffer.h"
#include <vulkan/vulkan.h>

#include "helper.h"
#include "../os/msg.h"

namespace vulkan{

Array<VertexBuffer*> vertex_buffers;
extern Device *default_device;

VkFormat _parse_vb_component_(const string &s) {
	VkVertexInputAttributeDescription a;
	if (s == "4f")
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	if (s == "3f")
		return VK_FORMAT_R32G32B32_SFLOAT;
	if (s == "2f")
		return VK_FORMAT_R32G32_SFLOAT;
	if (s == "4i")
		return VK_FORMAT_R32G32B32A32_SINT;
	if (s == "3i")
		return VK_FORMAT_R32G32B32_SINT;
	if (s == "2i")
		return VK_FORMAT_R32G32_SINT;
	msg_error("unknown vertex buffer component: " + s);
	return VK_FORMAT_R32G32B32_SFLOAT;
}

int format_size(VkFormat f);

Array<VkVertexInputAttributeDescription> parse_attr_descr(const string &format) {
	Array<VkVertexInputAttributeDescription> attribute_descriptions;

	auto xx = format.replace("|i", "").explode(",");
	int offset = 0;
	int index = 0;
	for (auto &f: xx) {
		VkVertexInputAttributeDescription a;
		a.binding = 0;
		a.location = index ++;
		a.format = _parse_vb_component_(f);
		a.offset = offset;
		offset += format_size(a.format);
		attribute_descriptions.add(a);
	}
	return attribute_descriptions;
}

VkVertexInputBindingDescription parse_binding_descr(const string &format) {
	VkVertexInputBindingDescription b;
	int offset = 0;
	for (auto &f: format.replace("|i", "").explode(","))
		offset += format_size(_parse_vb_component_(f));
	b.binding = 0;
	b.stride = offset;
	b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return b;
}


VertexBuffer::VertexBuffer(const string &format) :
		vertex_buffer(default_device),
		index_buffer(default_device)
{
	vertex_count = 0;
	output_count = 0;
	index_type = VK_INDEX_TYPE_UINT16;

	attribute_descriptions = parse_attr_descr(format);
	binding_description = parse_binding_descr(format);

	vertex_buffers.add(this);
}

VertexBuffer::~VertexBuffer() {
	_destroy();

	for (int i=0; i<vertex_buffers.num; i++)
		if (vertex_buffers[i] == this)
			vertex_buffers.erase(i);
}

void VertexBuffer::_destroy() {
	index_buffer.destroy();
	vertex_buffer.destroy();
	output_count = 0;
}

void VertexBuffer::update(const DynamicArray &vertices) {
	_create_buffer(vertex_buffer, vertices);
	vertex_count = vertices.num;
	output_count = vertices.num; // might be overwritten by creat_index_buffer()
}

void VertexBuffer::update_index(const Array<int> &indices) {
	_create_index_buffer_i32(indices);
}

void VertexBuffer::update_v3_v3_v2(const Array<Vertex1> &vertices) {
	update(vertices);
}

void VertexBuffer::_create_buffer(Buffer &buf, const DynamicArray &array) {
	if (array.num == 0)
		return;

	VkDeviceSize buffer_size = array.num * array.element_size;

	// -> staging
	Buffer staging(vertex_buffer.device);
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

bool VertexBuffer::is_indexed() const {
	return index_buffer.buffer;
}

int VertexBuffer::stride() const {
	return binding_description.stride;
}

void VertexBuffer::create_quad(const rect &r, const rect &s) {
	if (is_indexed()) {
		update_v3_v3_v2({
			{{r.x1,r.y1,0}, {0,0,1}, s.x1,s.y1},
			{{r.x2,r.y1,0}, {0,0,1}, s.x2,s.y1},
			{{r.x1,r.y2,0}, {0,0,1}, s.x1,s.y2},
			{{r.x2,r.y2,0}, {0,0,1}, s.x2,s.y2}});
		update_index({0,1,3, 0,3,2});
	} else {
		update_v3_v3_v2({
			{{r.x1,r.y1,0}, {0,0,1}, s.x1,s.y1},
			{{r.x2,r.y1,0}, {0,0,1}, s.x2,s.y1},
			{{r.x2,r.y2,0}, {0,0,1}, s.x2,s.y2},
			{{r.x1,r.y1,0}, {0,0,1}, s.x1,s.y1},
			{{r.x2,r.y2,0}, {0,0,1}, s.x2,s.y2},
			{{r.x1,r.y2,0}, {0,0,1}, s.x1,s.y2}});
	}
}

};

#endif
