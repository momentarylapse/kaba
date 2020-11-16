#ifndef _VULKAN_VULKAN_H
#define _VULKAN_VULKAN_H

#if HAS_LIB_VULKAN

#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>

#include "DescriptorSet.h"
#include "Buffer.h"
#include "VertexBuffer.h"
#include "Texture.h"
#include "Shader.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "CommandBuffer.h"
#include "FrameBuffer.h"
#include "SwapChain.h"
#include "Semaphore.h"
#include "AccelerationStructure.h"


VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);




namespace vulkan {

	class Instance {
	public:
		VkInstance instance;
		bool using_validation_layers;

		Instance();
		~Instance();
		void pick_physical_device();
		void create_surface(GLFWwindow* vulkan_window);

		void setup_debug_messenger();
		void _ensure_rtx();

		static Instance *create(const Array<string> &op);
	};


	//extern VkInstance instance;
	//extern VkDebugUtilsMessengerEXT debug_messenger;
	extern VkSurfaceKHR surface;

	extern VkPhysicalDevice physical_device;
	extern VkDevice device;

	extern VkQueue graphics_queue;
	extern VkQueue present_queue;


	VkFormat find_depth_format();
	void rebuild_pipelines();
	void queue_submit_command_buffer(CommandBuffer *cb, const Array<Semaphore*> &wait_sem, const Array<Semaphore*> &signal_sem, Fence *fence);


	GLFWwindow* create_window(const string &title, int width, int height);
	bool window_handle(GLFWwindow *window);
	void window_close(GLFWwindow *window);

	void init(GLFWwindow* window, const Array<string> &op);
	void destroy();


	void wait_device_idle();


	bool check_validation_layer_support();
	bool is_device_suitable(VkPhysicalDevice device);
	bool check_device_extension_support(VkPhysicalDevice device);
	void create_logical_device(bool validation);


	bool has_stencil_component(VkFormat format);


	namespace rtx {
		extern VkPhysicalDeviceRayTracingPropertiesNV properties;
		void get_properties();
	}


#define DECLARE_EXT_H(NAME) extern PFN_##NAME _##NAME;

	DECLARE_EXT_H(vkCmdTraceRaysNV);
	DECLARE_EXT_H(vkCmdBuildAccelerationStructureNV);
	DECLARE_EXT_H(vkCreateAccelerationStructureNV);
	DECLARE_EXT_H(vkDestroyAccelerationStructureNV);
	DECLARE_EXT_H(vkBindAccelerationStructureMemoryNV);
	DECLARE_EXT_H(vkCreateRayTracingPipelinesNV);
	DECLARE_EXT_H(vkGetAccelerationStructureMemoryRequirementsNV);
	DECLARE_EXT_H(vkGetAccelerationStructureHandleNV);
	DECLARE_EXT_H(vkGetRayTracingShaderGroupHandlesNV);
	DECLARE_EXT_H(vkGetPhysicalDeviceProperties2);
}

#endif

#endif
