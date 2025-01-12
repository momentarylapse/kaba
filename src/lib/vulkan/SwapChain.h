/*
 * SwapChain.h
 *
 *  Created on: 03.01.2020
 *      Author: michi
 */

#pragma once

#if HAS_LIB_VULKAN


#include "../base/base.h"
#include "../base/pointer.h"
#include "../image/color.h"
#include <vulkan/vulkan.h>
#include "FrameBuffer.h"


#ifdef HAS_LIB_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif


namespace vulkan {


class FrameBuffer;
class RenderPass;
class Semaphore;
class Device;

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	Array<VkSurfaceFormatKHR> formats;
	Array<VkPresentModeKHR> present_modes;
};
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);


class SwapChain {
public:
	//Array<VkImage> images;
	int width, height;
	VkSwapchainKHR swap_chain;
	VkFormat image_format;
	uint32_t image_count;
	Array<VkImageView> _image_views;
	Device *device;

	explicit SwapChain(Device *device);
	~SwapChain();

	void rebuild(int w, int h);

	static xfer<SwapChain> create(Device *device, int w, int h);
#ifdef HAS_LIB_GLFW
	static xfer<SwapChain> create_for_glfw(Device *device, GLFWwindow* window);
#endif

	Array<VkImage> get_images();
	Array<VkImageView> create_image_views(Array<VkImage> &images);

	xfer<DepthBuffer> create_depth_buffer();
	xfer<RenderPass> create_render_pass(DepthBuffer *depth_buffer, const Array<string> &options = {});
	Array<xfer<Texture>> create_textures();
	Array<xfer<FrameBuffer>> create_frame_buffers(RenderPass *rp, DepthBuffer *db);

	bool present(int image_index, const Array<Semaphore*> &wait_sem);
	bool acquire_image(int *image_index, Semaphore *signal_sem);
};



VkSurfaceFormatKHR choose_swap_surface_format(const Array<VkSurfaceFormatKHR>& available_formats);
VkPresentModeKHR choose_swap_present_mode(const Array<VkPresentModeKHR> available_present_modes);
VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);




} /* namespace vulkan */


#endif
