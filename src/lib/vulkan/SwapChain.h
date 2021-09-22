/*
 * SwapChain.h
 *
 *  Created on: 03.01.2020
 *      Author: michi
 */

#ifndef SRC_LIB_VULKAN_SWAPCHAIN_H_
#define SRC_LIB_VULKAN_SWAPCHAIN_H_

#if HAS_LIB_VULKAN


#include "../base/base.h"
#include "../image/color.h"
#include <vulkan/vulkan.h>
#include "FrameBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace vulkan {


class FrameBuffer;
class RenderPass;
class Semaphore;

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	Array<VkSurfaceFormatKHR> formats;
	Array<VkPresentModeKHR> present_modes;
};
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);


class SwapChain {
public:
	//Array<VkImage> images;
	int width, height;
	VkSwapchainKHR swap_chain;
	VkFormat image_format;
	uint32_t image_count;
	Array<VkImageView> _image_views;
	//Array<FrameBuffer*> frame_buffers;
	//DepthBuffer *depth_buffer;
	//RenderPass *default_render_pass;
	GLFWwindow* window;

	SwapChain(GLFWwindow* window);
	~SwapChain();

	void __init__(GLFWwindow* window);
	void __delete__();

	void cleanup();
	void create();

	Array<VkImage> get_images();
	Array<VkImageView> create_image_views(Array<VkImage> &images);

	DepthBuffer *create_depth_buffer();
	RenderPass *create_render_pass(DepthBuffer *depth_buffer);
	Array<Texture*> create_textures();
	Array<FrameBuffer*> create_frame_buffers(RenderPass *rp, DepthBuffer *db);

	void rebuild();

	bool present(int image_index, const Array<Semaphore*> &wait_sem);
	bool acquire_image(int *image_index, Semaphore *signal_sem);
};



VkSurfaceFormatKHR choose_swap_surface_format(const Array<VkSurfaceFormatKHR>& available_formats);
VkPresentModeKHR choose_swap_present_mode(const Array<VkPresentModeKHR> available_present_modes);
VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);



bool has_stencil_component(VkFormat format);


} /* namespace vulkan */


#endif

#endif /* SRC_LIB_VULKAN_SWAPCHAIN_H_ */
