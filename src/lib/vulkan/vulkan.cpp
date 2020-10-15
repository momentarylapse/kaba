#if HAS_LIB_VULKAN

#include "vulkan.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <array>

#include "helper.h"
#include "../base/base.h"
#include "../file/msg.h"
#include "../math/vector.h"
#include "../math/matrix.h"

//#define NDEBUG

Array<const char*> sa2pa(const Array<string> &sa) {
	Array<const char*> pa;
	for (string &s: sa)
		pa.add((const char*)s.data);
	return pa;
}



VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}



static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}




namespace vulkan {



	const std::vector<const char*> validation_layers = {
		//"VK_LAYER_LUNARG_standard_validation",
		"VK_LAYER_KHRONOS_validation",
	};

	const std::vector<const char*> instance_extensions = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	};

	const std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_NV_RAY_TRACING_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	};

	#ifdef NDEBUG
	bool enable_validation_layers = false;
	#else
	bool enable_validation_layers = true;
	#endif



	void setup_debug_messenger() {
		if (!enable_validation_layers) return;
		std::cout << " VALIDATION LAYER!\n";

		VkDebugUtilsMessengerCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = debug_callback;

		if (create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
			throw Exception("failed to set up debug messenger!");
		}
	}




GLFWwindow* vulkan_window;

VkInstance instance;
VkDebugUtilsMessengerEXT debug_messenger;
VkSurfaceKHR surface;

VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkPhysicalDeviceProperties device_properties;
VkDevice device;

VkQueue graphics_queue;
VkQueue present_queue;


#define DECLARE_EXT(NAME) PFN_##NAME p##NAME = nullptr;
#define LOAD_EXT(NAME) \
		p##NAME = (PFN_##NAME)vkGetInstanceProcAddr(instance, #NAME); \
		msg_write(format(" %s: %s", #NAME, p2s((void*)p##NAME))); \
		if (!p##NAME) \
			throw Exception("CAN NOT LOAD RTX EXTENSIONS");

DECLARE_EXT(vkCmdTraceRaysNV);
DECLARE_EXT(vkCreateRayTracingPipelinesNV);
DECLARE_EXT(vkCmdBuildAccelerationStructureNV);
DECLARE_EXT(vkCreateAccelerationStructureNV);
DECLARE_EXT(vkBindAccelerationStructureMemoryNV);
DECLARE_EXT(vkGetAccelerationStructureMemoryRequirementsNV);
DECLARE_EXT(vkGetAccelerationStructureHandleNV);
static bool rtx_loaded = false;


void ensure_rtx() {
	if (rtx_loaded)
		return;

/*	VkPhysicalDeviceRaytracingPropertiesNV mRTProps;
	mRTProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAYTRACING_PROPERTIES_NV;
	mRTProps.pNext = nullptr;
	mRTProps.maxRecursionDepth = 0;
	mRTProps.shaderHeaderSize = 0;

	VkPhysicalDeviceProperties2 devProps;
	devProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	devProps.pNext = &mRTProps;
	devProps.properties = { };

	vkGetPhysicalDeviceProperties2(device, &devProps);*/


	std::cout << "loading rtx extensions...\n";
	LOAD_EXT(vkCmdTraceRaysNV);
	LOAD_EXT(vkCreateRayTracingPipelinesNV);
	LOAD_EXT(vkCmdBuildAccelerationStructureNV);
	LOAD_EXT(vkCreateAccelerationStructureNV);
	LOAD_EXT(vkBindAccelerationStructureMemoryNV);
	LOAD_EXT(vkGetAccelerationStructureMemoryRequirementsNV);
	LOAD_EXT(vkGetAccelerationStructureHandleNV);

	if (!pvkCreateRayTracingPipelinesNV)
		std::cerr << "CAN NOT LOAD RTX EXTENSIONS\n";
	std::cout << " create pipeline: " << p2s((void*)pvkCreateRayTracingPipelinesNV).c_str() << "\n";

	rtx_loaded = true;
}


void init(GLFWwindow* window) {
	std::cout << "vulkan init\n";
	vulkan_window = window;
	try {

	create_instance();
	setup_debug_messenger();
	create_surface();
	pick_physical_device();
	create_logical_device();
	create_command_pool();

	descriptor_pool = create_descriptor_pool();

	ensure_rtx();
	} catch (Exception &e) {
		std::cerr << "ERROR: " << e.message().c_str() << "\n";
	}
}

extern Array<VertexBuffer*> vertex_buffers;
extern Array<Shader*> shaders;

void destroy() {
	std::cout << "vulkan destroy\n";

	auto _vertex_buffers = vertex_buffers;
	vertex_buffers.clear();
	for (auto *vb: _vertex_buffers)
		delete vb;

	auto _textures = textures;
	textures.clear();
	for (auto *t: _textures)
		delete t;

	auto _pipelines = pipelines;
	pipelines.clear();
	for (auto *p: _pipelines)
		delete p;

	auto _shaders = shaders;
	shaders.clear();
	for (auto *s: _shaders)
		delete s;

	destroy_descriptor_pool(descriptor_pool);

	destroy_command_pool();

	vkDestroyDevice(device, nullptr);

	if (enable_validation_layers) {
		destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);
	}

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}


bool check_validation_layer_support() {
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : validation_layers) {
		bool layer_found = false;

		for (const auto& layer_properties : available_layers) {
			if (strcmp(layer_name, layer_properties.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found) {
			return false;
		}
	}

	return true;
}
std::vector<const char*> get_required_instance_extensions() {
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	for (auto s: instance_extensions)
		extensions.push_back(s);

	if (enable_validation_layers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}



void create_instance() {
	if (enable_validation_layers and !check_validation_layer_support()) {
		//throw Exception("validation layers requested, but not available!");
		std::cout << "validation layers requested, but not available!" << '\n';
		enable_validation_layers = false;
	}

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Y-Engine";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Y-Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	auto extensions = get_required_instance_extensions();
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
		throw Exception("failed to create instance!");
	}
}

void create_surface() {
	if (glfwCreateWindowSurface(instance, vulkan_window, nullptr, &surface) != VK_SUCCESS) {
		throw Exception("failed to create window surface!");
	}
}

void pick_physical_device() {
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0) {
		throw Exception("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	for (const auto& device: devices) {
		if (is_device_suitable(device)) {
			physical_device = device;
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE) {
		throw Exception("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceProperties(physical_device, &device_properties);
	std::cout << "  minUniformBufferOffsetAlignment  " << device_properties.limits.minUniformBufferOffsetAlignment << "\n";
	std::cout << "  maxPushConstantsSize  " << device_properties.limits.maxPushConstantsSize << "\n";
	std::cout << "  maxImageDimension2D  " << device_properties.limits.maxImageDimension2D << "\n";
	std::cout << "  maxUniformBufferRange  " << device_properties.limits.maxUniformBufferRange << "\n";
	std::cout << "  maxPerStageDescriptorUniformBuffers  " << device_properties.limits.maxPerStageDescriptorUniformBuffers << "\n";
	std::cout << "  maxPerStageDescriptorSamplers  " << device_properties.limits.maxPerStageDescriptorSamplers << "\n";
	std::cout << "  maxDescriptorSetSamplers  " << device_properties.limits.maxDescriptorSetSamplers << "\n";
	std::cout << "  maxDescriptorSetUniformBuffers  " << device_properties.limits.maxDescriptorSetUniformBuffers << "\n";
	std::cout << "  maxDescriptorSetUniformBuffersDynamic  " << device_properties.limits.maxDescriptorSetUniformBuffersDynamic << "\n";
	//std::cout << "  maxDescriptorSetUniformBuffers  " << device_properties.limits.maxDescriptorSetUniformBuffers << "\n";
	//std::cout << "  maxDescriptorSetUniformBuffers  " << device_properties.limits.maxDescriptorSetUniformBuffers << "\n";

	/*VkPhysicalDeviceRayTracingFeaturesKHR rtf = {};

	VkPhysicalDeviceFeatures2 dp2 = {};
	dp2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	vkGetPhysicalDeviceFeatures2(physical_device, &dp2);*/
}


bool is_device_suitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = find_queue_families(device);

	bool extensions_supported = check_device_extension_support(device);

	bool swap_chain_adequate = false;
	if (extensions_supported) {
		SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device);
		swap_chain_adequate = (swapChainSupport.formats.num > 0) and (swapChainSupport.present_modes.num > 0);
	}
	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(device, &supported_features);

	return indices.is_complete() and extensions_supported and swap_chain_adequate and supported_features.samplerAnisotropy;
}

bool check_device_extension_support(VkPhysicalDevice device) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	std::cout << "---- GPU-----\n";
	for (const auto& extension : available_extensions) {
		std::cout << "   " << extension.extensionName << "\n";
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}


void create_logical_device() {
	QueueFamilyIndices indices = find_queue_families(physical_device);

	Array<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.add(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	create_info.queueCreateInfoCount = queue_create_infos.num;
	create_info.pQueueCreateInfos = &queue_create_infos[0];

	create_info.pEnabledFeatures = &device_features;

	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
		throw Exception("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
	vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}







void rebuild_pipelines() {
	for (auto *p: pipelines)
		p->rebuild();
}


void queue_submit_command_buffer(CommandBuffer *cb, const Array<Semaphore*> &wait_sem, const Array<Semaphore*> &signal_sem, Fence *fence) {

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	auto wait_semaphores = extract_semaphores(wait_sem);
	auto signal_semaphores = extract_semaphores(signal_sem);
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = wait_semaphores.num;
	submit_info.pWaitSemaphores = &wait_semaphores[0];
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cb->buffer;
	submit_info.signalSemaphoreCount = signal_semaphores.num;
	submit_info.pSignalSemaphores = &signal_semaphores[0];


	if (fence)
		fence->reset();

	VkResult result = vkQueueSubmit(graphics_queue, 1, &submit_info, fence_handle(fence));
	if (result != VK_SUCCESS) {
		std::cerr << " SUBMIT ERROR " << result << "\n";
		throw Exception("failed to submit draw command buffer!");
	}
}


void wait_device_idle() {
	vkDeviceWaitIdle(device);
}






GLFWwindow* create_window(const string &title, int width, int height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	//glfwSetWindowUserPointer(window, this);
	return window;
}

bool window_handle(GLFWwindow *window) {
	if (glfwWindowShouldClose(window))
		return true;
	glfwPollEvents();
	return false;
}

void window_close(GLFWwindow *window) {
	glfwDestroyWindow(window);

	glfwTerminate();
}

}

#endif
