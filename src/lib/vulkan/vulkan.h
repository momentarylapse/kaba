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

extern const int MAX_FRAMES_IN_FLIGHT;


VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);




struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};



namespace vulkan {


	extern VkInstance instance;
	extern VkDebugUtilsMessengerEXT debug_messenger;
	extern VkSurfaceKHR surface;

	extern VkPhysicalDevice physical_device;
	extern VkDevice device;

	extern VkQueue graphics_queue;
	extern VkQueue present_queue;

	extern bool enable_validation_layers;



	extern size_t current_frame;
	extern bool framebuffer_resized;

	extern GLFWwindow* vulkan_window;
	extern int target_width, target_height;

	GLFWwindow* create_window(const string &title, int width, int height);
	bool window_handle(GLFWwindow *window);
	void window_close(GLFWwindow *window);

	void init(GLFWwindow* window);
	void destroy();

	void on_resize(int with, int height);

	bool start_frame();
	void end_frame();

	void wait_device_idle();

	void submit_command_buffer(CommandBuffer *cb);



	void setup_debug_messenger();

	void cleanup_swap_chain();
	void create_instance();
	bool check_validation_layer_support();
	std::vector<const char*> get_required_extensions();
	void create_surface();
	void pick_physical_device();
	bool is_device_suitable(VkPhysicalDevice device);
	bool check_device_extension_support(VkPhysicalDevice device);
	SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
	void create_logical_device();
	void create_swap_chain();

	VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);


	void create_framebuffers(RenderPass *rp);
	void create_image_views();

	void create_depth_resources();
	bool has_stencil_component(VkFormat format);


	void create_sync_objects();
	void recreateSwapChain(GLFWwindow* window);

}

#endif

#endif
