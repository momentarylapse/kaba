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

	class RenderPass {
	public:
		RenderPass(const Array<VkFormat> &format, bool clear, bool presentable);
		~RenderPass();

		void __init__(const Array<VkFormat> &format, bool clear, bool representable);
		void __delete__();

		void create();
		void destroy();
		void rebuild();

		VkRenderPass render_pass;
		Array<color> clear_color;
		float clear_z;
		unsigned int clear_stencil;

		int num_color_attachments() { return color_attachment_refs.num; }

	private:
		Array<VkAttachmentDescription> attachments;
		Array<VkAttachmentReference> color_attachment_refs;
		VkAttachmentReference depth_attachment_ref;
		VkSubpassDescription subpass;
		Array<VkSubpassDependency> dependencies;
	};
};

#endif

#endif /* RenderPass_hpp */
