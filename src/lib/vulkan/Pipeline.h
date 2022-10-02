//
//  Pipeline.hpp
//   * vertex layout
//       (num texture coords)
//   * shader
//   * rendering parameters
//      - z
//      - stencil op
//      - blending
//      - viewport
//      - rasterizer (fill / wire)
//      - which color component to write
//
//  Created by <author> on 06/02/2019.
//
//

#pragma once

#if HAS_LIB_VULKAN


#include "../base/base.h"
#include <vulkan/vulkan.h>
#include "Buffer.h"

class rect;

namespace vulkan{

	class Shader;
	class VertexBuffer;
	class RenderPass;


	enum class Alpha {
		ZERO             = VK_BLEND_FACTOR_ZERO,
		ONE              = VK_BLEND_FACTOR_ONE,
		SOURCE_COLOR     = VK_BLEND_FACTOR_SRC_COLOR,
		SOURCE_INV_COLOR = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		SOURCE_ALPHA     = VK_BLEND_FACTOR_SRC_ALPHA,
		SOURCE_INV_ALPHA = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		DEST_COLOR       = VK_BLEND_FACTOR_DST_COLOR,
		DEST_INV_COLOR   = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		DEST_ALPHA       = VK_BLEND_FACTOR_DST_ALPHA,
		DEST_INV_ALPHA   = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
	};

	enum class CullMode {
		NONE = VK_CULL_MODE_NONE,
		BACK = VK_CULL_MODE_BACK_BIT,
		FRONT = VK_CULL_MODE_FRONT_BIT
	};

	class BasePipeline {
	public:
		BasePipeline(VkPipelineBindPoint bind_point, Shader *shader);
		BasePipeline(VkPipelineBindPoint bind_point, const Array<VkDescriptorSetLayout> &dset_layouts);
		~BasePipeline();

		void destroy();

		Shader *shader = nullptr;
		Array<VkDescriptorSetLayout> descr_layouts;
		VkPipelineLayout layout = VK_NULL_HANDLE;
		Array<VkPipelineShaderStageCreateInfo> shader_stages;

		VkPipeline pipeline = VK_NULL_HANDLE;

		VkPipelineBindPoint bind_point;


		//static VkPipelineLayout create_layout(const Array<VkDescriptorSetLayout> &dset_layouts);
	};

	class GraphicsPipeline : public BasePipeline {
	public:
		GraphicsPipeline(Shader *shader, RenderPass *render_pass, int subpass, const string &topology, VkVertexInputBindingDescription binding_description, const Array<VkVertexInputAttributeDescription> &attribute_descriptions);
		GraphicsPipeline(Shader *shader, RenderPass *render_pass, int subpass, const string &topology, const string &format);
		GraphicsPipeline(Shader *shader, RenderPass *render_pass, int subpass, const string &topology, VertexBuffer *vb);
		~GraphicsPipeline();

		void rebuild();

		// configuration
		void disable_blend();
		void set_blend(Alpha src, Alpha dst);
		void set_blend(float factor);
		void set_wireframe(bool wireframe);
		void set_line_width(float line_width);
		void set_z(bool test, bool write);
		void set_viewport(const rect &r);
		void set_culling(CullMode mode);

		void set_dynamic(const Array<string> &dynamic_states);

		RenderPass *render_pass;
		int subpass;
		VkViewport viewport;

	private:

		Array<VkDynamicState> dynamic_states;

		VkVertexInputBindingDescription binding_description;
		Array<VkVertexInputAttributeDescription> attribute_descriptions;
		VkPipelineVertexInputStateCreateInfo vertex_input_info;
		Array<VkPipelineColorBlendAttachmentState> color_blend_attachments; // per FrameBuffer color attachment
		VkPipelineColorBlendStateCreateInfo color_blending;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineDepthStencilStateCreateInfo depth_stencil;
		VkPipelineInputAssemblyStateCreateInfo input_assembly;
	};



	class ComputePipeline : public BasePipeline {
	public:
		ComputePipeline(const string &dset_layouts, Shader *shader);
	};

	class RayPipeline : public BasePipeline {
	public:
		RayPipeline(const string &dset_layouts, const Array<Shader*> &shaders, int recursion_depth);

		void create_groups(const Array<Shader*> &shaders);
		void create_sbt();

		int miss_group_offset;
	    Array<VkRayTracingShaderGroupCreateInfoNV> groups;
	    Buffer sbt;
	};

};

#endif
