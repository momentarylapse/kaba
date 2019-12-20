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


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);




struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};



namespace vulkan {


	extern VkInstance instance;
	extern VkDebugUtilsMessengerEXT debugMessenger;
	extern VkSurfaceKHR surface;

	extern VkPhysicalDevice physicalDevice;
	extern VkDevice device;

	extern VkQueue graphicsQueue;
	extern VkQueue presentQueue;

	extern bool enableValidationLayers;



	extern size_t currentFrame;
	extern bool framebufferResized;

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



	void setupDebugMessenger();

	void cleanupSwapChain();
	void createInstance();
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	void createSurface();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	void createLogicalDevice();
	void createSwapChain();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);


	void createFramebuffers(RenderPass *rp);
	void createImageViews();

	void createDepthResources();
	bool hasStencilComponent(VkFormat format);


	void createSyncObjects();
	void recreateSwapChain(GLFWwindow* window);

}

#endif

#endif
