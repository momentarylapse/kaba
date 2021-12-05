#ifndef _VULKAN_VERTEXBUFFER_H
#define _VULKAN_VERTEXBUFFER_H

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include "../math/vector.h"
#include <vector>
#include <vulkan/vulkan.h>
#include "Buffer.h"

namespace vulkan {

	struct Vertex1 {
		vector pos;
		vector normal;
		float u,v;
		//Vertex1() {}
		//Vertex1(const vector &p, const vector &n, float u, float v);
	};

	class VertexBuffer {
	public:
		VertexBuffer(const string &format);
		~VertexBuffer();

		void __init__(const string &format);
		void __delete__();

		void _create_buffer(Buffer &buf, const DynamicArray &array);
		void _create_index_buffer_i16(const Array<uint16_t> &indices);
		void _create_index_buffer_i32(const Array<int> &indices);

		void _destroy();

		unsigned int output_count;
		unsigned int vertex_count;
		VkIndexType index_type;
		Array<VkVertexInputAttributeDescription> attribute_descriptions;
		VkVertexInputBindingDescription binding_description;

		Buffer vertex_buffer;
		Buffer index_buffer;

		void update(const DynamicArray &vertices);
		void update_i(const DynamicArray &vertices, const Array<int> &indices);
		void update_v3_v3_v2(const Array<Vertex1> &vertices);
		void update_v3_v3_v2_i(const Array<Vertex1> &vertices, const Array<int> &indices);
	};
};

#endif

#endif
