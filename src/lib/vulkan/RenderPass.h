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
#include <vulkan/vulkan.h>
#include <vector>

namespace vulkan{

	class RenderPass {
	public:
		RenderPass(VkAttachmentLoadOp color_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR, VkAttachmentLoadOp depth_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR);
		~RenderPass();

		void __init__();
		void __delete__();

		void create();
		void destroy();

		VkRenderPass render_pass;

	private:
		VkAttachmentDescription color_attachment;
		VkAttachmentDescription depth_attachment;
		VkAttachmentReference color_attachment_ref;
		VkAttachmentReference depth_attachment_ref;
		VkSubpassDescription subpass;
		VkSubpassDependency dependency;
	};
};

#endif

#endif /* RenderPass_hpp */
