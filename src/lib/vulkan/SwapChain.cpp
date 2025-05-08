/*
 * SwapChain.cpp
 *
 *  Created on: 03.01.2020
 *      Author: michi
 */

#if HAS_LIB_VULKAN


#include "SwapChain.h"
#include "vulkan.h"
#include "helper.h"

#include <iostream>
#include "../os/msg.h"

namespace vulkan {

	string result2str(VkResult r);



VkSurfaceFormatKHR choose_swap_surface_format(const Array<VkSurfaceFormatKHR>& available_formats) {
	if (available_formats.num == 1 and available_formats[0].format == VK_FORMAT_UNDEFINED)
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

	for (const auto& format: available_formats)
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM and format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;

	return available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(const Array<VkPresentModeKHR> available_present_modes) {
	VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& mode: available_present_modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			best_mode = mode;
	}

	return best_mode;
}

#ifdef HAS_LIB_GLFW
VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {

		int win_width = 0;
		int win_height = 0;
		while (win_width == 0 or win_height == 0) {
			glfwWaitEvents();
			glfwGetFramebufferSize(window, &win_width, &win_height);
		}

		VkExtent2D actual_extent = {(unsigned)win_width, (unsigned)win_height};

		actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}
#endif


Array<xfer<Texture>> SwapChain::create_textures() {
	Array<xfer<Texture>> textures;
	auto images = get_images();
	auto image_views = create_image_views(images);
	for (int i=0; i<images.num; i++) {
		auto t = new Texture();
		t->width = width;
		t->height = height;
		t->image.format = image_format;
		t->image.image = images[i];
		t->view = image_views[i];
		textures.add(t);
	}
	return textures;
}


Array<xfer<FrameBuffer>> SwapChain::create_frame_buffers(RenderPass *render_pass, DepthBuffer *depth_buffer) {
	Array<xfer<FrameBuffer>> frame_buffers;
	auto textures = create_textures();

	for (size_t i=0; i<image_count; i++)
		frame_buffers.add(new FrameBuffer(render_pass, {textures[i], depth_buffer}));

	return frame_buffers;
}


SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, &details.formats[0]);
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, &details.present_modes[0]);
	}

	return details;
}

xfer<DepthBuffer> SwapChain::create_depth_buffer() {
	return new DepthBuffer(width, height, device->find_depth_format(), false);
}

xfer<RenderPass> SwapChain::create_render_pass(DepthBuffer *depth_buffer, const Array<string> &options) {
	auto _options = options;
	_options.add("present");
	return new RenderPass({image_format, depth_buffer->image.format}, _options);
}


void SwapChain::rebuild(int w, int h) {
	width = w;
	height = h;

	const SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device->physical_device, device->surface);

	const VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
	const VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
	const VkExtent2D extent = {(uint32_t)width, (uint32_t)height};

	image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 and image_count > swap_chain_support.capabilities.maxImageCount)
		image_count = swap_chain_support.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = device->surface;

	info.oldSwapchain = swap_chain;
	info.minImageCount = image_count;
	info.imageFormat = surface_format.format;
	info.imageColorSpace = surface_format.colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = device->indices;
	uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

	if (indices.graphics_family.value() != indices.present_family.value()) {
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = queue_family_indices;
	} else {
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	info.preTransform = swap_chain_support.capabilities.currentTransform;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = present_mode;
	info.clipped = VK_TRUE;

	auto r = vkCreateSwapchainKHR(device->device, &info, nullptr, &swap_chain);
	if (r != VK_SUCCESS)
		throw Exception("failed to create swap chain!  " + result2str(r));

	image_format = surface_format.format;
}


xfer<SwapChain> SwapChain::create(Device *device, int width, int height) {
	auto swap_chain = new SwapChain(device);
	swap_chain->rebuild(width, height);
	return swap_chain;
}

#ifdef HAS_LIB_GLFW
xfer<SwapChain> SwapChain::create_for_glfw(Device *device, GLFWwindow* window) {
	const SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device->physical_device, device->surface);
	auto extent = choose_swap_extent(swap_chain_support.capabilities, window);
	return create(device, (int)extent.width, (int)extent.height);
}
#endif

// well, no memory :P
Array<VkImage> SwapChain::get_images() {
	vkGetSwapchainImagesKHR(device->device, swap_chain, &image_count, nullptr);
	Array<VkImage> images;
	images.resize((int)image_count);
	vkGetSwapchainImagesKHR(device->device, swap_chain, &image_count, &images[0]);
	return images;
}





Array<VkImageView> SwapChain::create_image_views(Array<VkImage> &images) {
	Array<VkImageView> views;
	for (uint32_t i=0; i<image_count; i++)
		views.add(ImageAndMemory{images[i], nullptr, image_format}.create_view(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 0, 1));
	_image_views = views;
	return views;
}



SwapChain::SwapChain(Device *d) {
	device = d;
}

SwapChain::~SwapChain() {
/*	for (auto frame_buffer: frame_buffers)
		delete frame_buffer;
	frame_buffers.clear();
	images.clear(); // only references anyways

	if (depth_buffer)
		delete depth_buffer;
	depth_buffer = nullptr;

	for (auto v: _image_views)
		vkDestroyImageView(device, v, nullptr);
	_image_views.clear();*/

	vkDestroySwapchainKHR(device->device, swap_chain, nullptr);
}

bool SwapChain::present(int image_index, const Array<Semaphore*> &wait_sem) {
	auto wait_semaphores = extract_semaphores(wait_sem);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = wait_semaphores.num;
	present_info.pWaitSemaphores = &wait_semaphores[0];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swap_chain;
	present_info.pImageIndices = (unsigned int*)&image_index;

	VkResult result = vkQueuePresentKHR(device->present_queue.queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR)
		return false;
	if (result != VK_SUCCESS)
		throw Exception("failed to present swap chain image!  " + result2str(result));
	return true;
}

bool SwapChain::acquire_image(int *image_index, Semaphore *signal_sem) {
	VkResult result = vkAcquireNextImageKHR(device->device, swap_chain, std::numeric_limits<uint64_t>::max(), signal_sem->semaphore, VK_NULL_HANDLE, (unsigned int*)image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		return false;
	if (result != VK_SUCCESS and result != VK_SUBOPTIMAL_KHR)
		throw Exception("failed to acquire swap chain image!  " + result2str(result));
	return true;
}

} /* namespace vulkan */

#endif

