//
//  Pipeline.hpp
//  3
//
//  Created by <author> on 06/02/2019.
//
//

#ifndef Pipeline_hpp
#define Pipeline_hpp

#if HAS_LIB_VULKAN


#include "../base/base.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace vulkan{

	class Shader;
	class RenderPass;

	class Pipeline {
	public:
		Pipeline(Shader *shader, RenderPass *render_pass);
		~Pipeline();

		void __init__(Shader *shader, RenderPass *render_pass);
		void __delete__();

		void create();
		void destroy();

		static Pipeline* build(Shader *shader, RenderPass *render_pass, bool create = true);

		// configuration
		void disable_blend();
		void set_blend(VkBlendFactor src, VkBlendFactor dst);
		void set_blend(float factor);
		void set_wireframe(bool wireframe);
		void set_line_width(float line_width);
		void set_z(bool test, bool write);

		void set_dynamic(const Array<VkDynamicState> &dynamic_states);

		Shader *shader;
		RenderPass *render_pass;
		Array<VkDescriptorSetLayout> descr_layouts;

		VkPipelineLayout layout;
		VkPipeline pipeline;

	private:

		Array<VkDynamicState> dynamic_states;

		VkVertexInputBindingDescription binding_description;
		std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
		std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
		VkPipelineVertexInputStateCreateInfo vertex_input_info;
		VkPipelineColorBlendAttachmentState color_blend_attachment;
		VkPipelineColorBlendStateCreateInfo color_blending;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineDepthStencilStateCreateInfo depth_stencil;
		VkPipelineInputAssemblyStateCreateInfo input_assembly;
	};

	extern std::vector<Pipeline*> pipelines;

};

#endif

#endif /* Pipeline_hpp */
