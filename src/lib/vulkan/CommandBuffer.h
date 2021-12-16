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

class rect;
class color;

namespace vulkan{

	class BasePipeline;
	class VertexBuffer;
	class RenderPass;
	class DescriptorSet;
	class FrameBuffer;
	class Texture;

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
		VkCommandBuffer buffer;

		VkPipelineBindPoint cur_bind_point;
		BasePipeline *current_pipeline;
		FrameBuffer *current_framebuffer;

		void begin();
		void end();
		void submit();

		void bind_pipeline(BasePipeline *p);
		void bind_descriptor_set(int index, DescriptorSet *dset);
		void bind_descriptor_set_dynamic(int index, DescriptorSet *dset, const Array<int> &indices);
		void push_constant(int offset, int size, void *data);

		void begin_render_pass(RenderPass *rp, FrameBuffer *fb);
		void next_subpass();
		void end_render_pass();
		void clear(const Array<color> &col, float z, bool clear_z);
		void draw(VertexBuffer *vb);

		void set_bind_point(const string &s);

		void set_scissor(const rect &r);
		void set_viewport(const rect &r);

		void dispatch(int nx, int ny, int nz);
		void trace_rays(int nx, int ny, int nz);

		void barrier(const Array<Texture*> &t, int mode);
		void image_barrier(const Texture *t, const Array<int> &flags);
		void copy_image(const Texture *source, const Texture *dest, const Array<int> &extend);

		void timestamp(int id);
	};

	VkCommandBuffer begin_single_time_commands();
	void end_single_time_commands(VkCommandBuffer commandBuffer);


};

#endif

#endif /* CommandBuffer_hpp */
