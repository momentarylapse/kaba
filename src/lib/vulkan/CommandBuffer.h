//
//  CommandBuffer.hpp
//  3
//
//  Created by <author> on 06/02/2019.
//
//

#ifndef CommandBuffer_hpp
#define CommandBuffer_hpp

#if HAS_LIB_VULKAN

#include <vulkan/vulkan.h>
#include "../base/base.h"

class color;
class rect;

namespace vulkan{

	class Pipeline;
	class VertexBuffer;
	class RenderPass;
	class DescriptorSet;

	extern VkCommandPool command_pool;

	void create_command_pool();
	void destroy_command_pool();

	class CommandBuffer {
	public:
		CommandBuffer();
		~CommandBuffer();

		void __init__();
		void __delete__();

		void _create();
		void _destroy();
		Array<VkCommandBuffer> buffers;
		VkCommandBuffer current;

		Pipeline *current_pipeline;

		void begin();
		void end();
		void submit();
		void set_pipeline(Pipeline *p);
		void bind_descriptor_set(int index, DescriptorSet *dset);
		void push_constant(int offset, int size, void *data);
		void begin_render_pass(RenderPass *rp, const color &clear_color);
		void end_render_pass();
		void draw(VertexBuffer *vb);
		void scissor(const rect &r);
	};

	VkCommandBuffer begin_single_time_commands();
	void end_single_time_commands(VkCommandBuffer commandBuffer);


};

#endif

#endif /* CommandBuffer_hpp */
