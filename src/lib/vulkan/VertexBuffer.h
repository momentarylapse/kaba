#ifndef _VULKAN_VERTEXBUFFER_H
#define _VULKAN_VERTEXBUFFER_H

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

		void _create_vertex_buffer(void *vdata, int size);
		void _create_index_buffer(const Array<uint16_t> &indices);

		unsigned int output_count;

		VkBuffer vertex_buffer;
		VkDeviceMemory vertex_memory;
		VkBuffer index_buffer;
		VkDeviceMemory index_memory;

		static VertexBuffer* build1(const Array<Vertex1> &vertices);
		static VertexBuffer* build1i(const Array<Vertex1> &vertices, const Array<uint16_t> &indices);
	};
};

#endif
