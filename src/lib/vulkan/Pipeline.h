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

#ifndef Pipeline_hpp
#define Pipeline_hpp

#if HAS_LIB_VULKAN


#include "../base/base.h"
#include <vulkan/vulkan.h>

class rect;

namespace vulkan{

	class Shader;
	class RenderPass;

	class BasePipeline {
	public:
		BasePipeline(Shader *shader);
		~BasePipeline();

		void destroy();

		Shader *shader;
		Array<VkDescriptorSetLayout> descr_layouts;
		VkPipelineLayout layout;
		Array<VkPipelineShaderStageCreateInfo> shader_stages;

		VkPipeline pipeline;
	};

	class Pipeline : public BasePipeline {
	public:
		Pipeline(Shader *shader, RenderPass *render_pass, int subpass, int num_textures);
		~Pipeline();

		void __init__(Shader *shader, RenderPass *render_pass, int subpass, int num_textures);
		void __delete__();

		void rebuild();

		// configuration
		void disable_blend();
		void set_blend(VkBlendFactor src, VkBlendFactor dst);
		void set_blend(float factor);
		void set_wireframe(bool wireframe);
		void set_line_width(float line_width);
		void set_z(bool test, bool write);
		void set_viewport(const rect &r);
		void set_culling(int mode);

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



	class RayPipeline : public BasePipeline {
	public:
		RayPipeline(Shader *shader);
	};

	extern Array<Pipeline*> pipelines;

};

#endif

#endif /* Pipeline_hpp */
