#ifndef _VULKAN_VULKAN_H
#define _VULKAN_VULKAN_H

#if HAS_LIB_VULKAN

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>

#include "VertexBuffer.h"
#include "Texture.h"
#include "Shader.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "CommandBuffer.h"
#include "FrameBuffer.h"
#include "SwapChain.h"
#include "Semaphore.h"


VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);




namespace vulkan {


	extern VkInstance instance;
	extern VkDebugUtilsMessengerEXT debug_messenger;
	extern VkSurfaceKHR surface;

	extern VkPhysicalDevice physical_device;
	extern VkDevice device;

	extern VkQueue graphics_queue;
	extern VkQueue present_queue;

	extern bool enable_validation_layers;


	extern GLFWwindow* vulkan_window;


	VkFormat find_depth_format();
	void rebuild_pipelines();
	void queue_submit_command_buffer(CommandBuffer *cb, const Array<Semaphore*> &wait_sem, const Array<Semaphore*> &signal_sem, Fence *fence);


	GLFWwindow* create_window(const string &title, int width, int height);
	bool window_handle(GLFWwindow *window);
	void window_close(GLFWwindow *window);

	void init(GLFWwindow* window);
	void destroy();


	void wait_device_idle();



	void setup_debug_messenger();

	void create_instance();
	bool check_validation_layer_support();
	std::vector<const char*> get_required_extensions();
	void create_surface();
	void pick_physical_device();
	bool is_device_suitable(VkPhysicalDevice device);
	bool check_device_extension_support(VkPhysicalDevice device);
	void create_logical_device();


	bool has_stencil_component(VkFormat format);

}

#endif

#endif
