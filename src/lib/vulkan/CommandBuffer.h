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
#include "../base/optional.h"

struct rect;
struct color;

namespace vulkan{

	class BasePipeline;
	class VertexBuffer;
	class RenderPass;
	class DescriptorSet;
	class FrameBuffer;
	class Texture;
	class Device;
	class CommandBuffer;


	enum class AccessFlags {
		NONE = 0, // VK_ACCESS_NONE,
		SHADER_READ_BIT = VK_ACCESS_SHADER_READ_BIT,
		SHADER_WRITE_BIT = VK_ACCESS_SHADER_WRITE_BIT,
		TRANSFER_READ_BIT = VK_ACCESS_TRANSFER_READ_BIT,
		TRANSFER_WRITE_BIT = VK_ACCESS_TRANSFER_WRITE_BIT
	};

	enum class ImageLayout {
		UNDEFINED = VK_IMAGE_LAYOUT_UNDEFINED,
		GENERAL = VK_IMAGE_LAYOUT_GENERAL,
		TRANSFER_SRC_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		TRANSFER_DST_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		SHADER_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		PRESENT_SRC = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		COLOR_ATTACHMENT = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	enum class PipelineBindPoint {
		GRAPHICS = VK_PIPELINE_BIND_POINT_GRAPHICS,
		RAY_TRACING = VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
		COMPUTE = VK_PIPELINE_BIND_POINT_COMPUTE
	};

	class CommandPool {
	public:
		explicit CommandPool(Device *device);
		~CommandPool();

		VkCommandPool command_pool;
		Device *device;

		CommandBuffer *create_command_buffer();
	};

	class CommandBuffer {
	public:
		explicit CommandBuffer(CommandPool *pool);
		~CommandBuffer();

		VkCommandBuffer buffer;
		CommandPool *pool;

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
		void clear(const rect& area, const Array<color> &col, base::optional<float> z);
		void draw(VertexBuffer *vb);
		void draw_instanced(VertexBuffer *vb, int num_instances);

		void set_bind_point(PipelineBindPoint bind_point);

		void set_scissor(const rect &r);
		void set_viewport(const rect &r);

		void dispatch(int nx, int ny, int nz);
		void trace_rays(int nx, int ny, int nz);

		void barrier(const Array<Texture*> &t, int mode);
		void image_barrier(const Texture *t, AccessFlags src_access, AccessFlags dst_access, ImageLayout old_layout, ImageLayout new_layout);
		void copy_image(const Texture *source, const Texture *dest, const Array<int> &extend);

		void timestamp(int id, bool after);
	};

	CommandBuffer *begin_single_time_commands();
	void end_single_time_commands(CommandBuffer *cb);


};

#endif

#endif /* CommandBuffer_hpp */
