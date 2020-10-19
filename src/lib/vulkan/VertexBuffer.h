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
		Vertex1() {}
		Vertex1(const vector &p, const vector &n, float u, float v);

		static VkVertexInputBindingDescription binding_description();
		static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
	};

	class VertexBuffer {
	public:
		VertexBuffer();
		~VertexBuffer();

		void __init__();
		void __delete__();

		void _create_vertex_buffer(const void *vdata, int size);
		void _create_index_buffer(const Array<uint16_t> &indices);

		void _destroy();

		unsigned int output_count;

		Buffer vertex_buffer;
		Buffer index_buffer;

		void build(const void *vertices, int size, int count);
		void build1(const Array<Vertex1> &vertices);
		void build1i(const Array<Vertex1> &vertices, const Array<int> &indices);
	};
};

#endif

#endif
