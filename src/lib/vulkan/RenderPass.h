//
//  RenderPass.hpp
//   * load/store color/depth buffers
//
//  Created by <author> on 06/02/2019.
//
//

#ifndef RenderPass_hpp
#define RenderPass_hpp

#if HAS_LIB_VULKAN



#include "../base/base.h"
#include "../image/color.h"
#include <vulkan/vulkan.h>

namespace vulkan{

	class Texture;
	class CommandBuffer;

	class RenderPass {
		friend CommandBuffer;
	public:
		RenderPass(const Array<Texture*> &images, const Array<string> &options = {});
		RenderPass(const Array<string> &formats, const Array<string> &options = {});
		RenderPass(const Array<VkFormat> &formats, const Array<string> &options = {});
		~RenderPass();

		void create();
		void destroy();
		void rebuild();

		void add_subpass(const Array<int> &color__att, int depth_att);
		void _add_dependency(int src, VkPipelineStageFlagBits src_stage, VkAccessFlags src_access, int dst, VkPipelineStageFlagBits dst_stage, VkAccessFlags dst_access);
		void add_dependency(int src, const string &src_opt, int dst, const string &dst_opt);

		VkRenderPass render_pass;
		Array<color> clear_color;
		float clear_z;
		unsigned int clear_stencil;

		int num_subpasses() { return subpass_data.num; }
		int num_color_attachments(int sub) { return subpass_data[sub].color_attachment_refs.num; }

	private:

		// common
		Array<VkAttachmentDescription> attachments;

		bool auto_subpasses, auto_dependencies;
		struct SubpassData {
			Array<VkAttachmentReference> color_attachment_refs;
			VkAttachmentReference depth_attachment_ref;
		};
		Array<SubpassData> subpass_data;
		Array<VkSubpassDescription> subpasses;
		Array<VkSubpassDependency> dependencies;
	};
};

#endif

#endif /* RenderPass_hpp */
