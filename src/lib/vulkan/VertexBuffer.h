#ifndef _VULKAN_VERTEXBUFFER_H
#define _VULKAN_VERTEXBUFFER_H

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include "../math/vector.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan {

	struct Vertex1 {
		vector pos;
		vector normal;
		float u,v;

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

		unsigned int output_count;

		VkBuffer vertex_buffer;
		VkDeviceMemory vertex_memory;
		VkDeviceSize vertex_buffer_size;
		VkBuffer index_buffer;
		VkDeviceMemory index_memory;
		VkDeviceSize index_buffer_size;

		void build(const void *vertices, int size, int count);
		void build1(const Array<Vertex1> &vertices);
		void build1i(const Array<Vertex1> &vertices, const Array<int> &indices);
	};
};

#endif

#endif
