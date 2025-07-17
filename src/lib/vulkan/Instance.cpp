/*
 * Instance.cpp
 *
 *  Created on: Sep 21, 2021
 *      Author: michi
 */

#if HAS_LIB_VULKAN

#include "vulkan.h"
#include "Instance.h"
#include "common.h"
#include "../os/msg.h"




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
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}



static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	msg_write(format("VALIDATION LAYER:  %s", pCallbackData->pMessage));
	return VK_FALSE;
}



namespace vulkan {


	bool check_validation_layer_support();

	Array<const char*> validation_layers = {
		//"VK_LAYER_LUNARG_standard_validation",
		"VK_LAYER_KHRONOS_validation",
	};


Array<const char*> get_required_instance_extensions(bool glfw, bool validation, bool headless) {
	Array<const char*> extensions;

	// default
	extensions.add(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef OS_MAC
	extensions.add("VK_KHR_portability_enumeration");
#endif

#ifdef HAS_LIB_GLFW
	if (glfw) {
		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		for (uint32_t i=0; i<glfw_extension_count; i++)
			extensions.add(glfw_extensions[i]);
	}
#endif

	if (headless) {
		extensions.add("VK_EXT_headless_surface"); // VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME
		extensions.add("VK_KHR_surface"); // VK_KHR_SURFACE_EXTENSION_NAME
	}

	if (validation)
		extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

	string result2str(VkResult r) {
		if (r == VK_ERROR_SURFACE_LOST_KHR)
			return "VK_ERROR_SURFACE_LOST_KHR";
		if (r == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		return "err=" + str((int)r);
	}


	VkDebugUtilsMessengerEXT debug_messenger;

	void Instance::setup_debug_messenger() {
		if (verbosity >= 2)
			msg_write(" VALIDATION LAYER!");

		VkDebugUtilsMessengerCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = debug_callback;

		auto r = create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger);
		if (r != VK_SUCCESS)
			throw Exception("failed to set up debug messenger!  " + result2str(r));
	}



Instance::Instance() {
	instance = nullptr;
	using_validation_layers = false;
}

Instance::~Instance() {
	if (verbosity >= 1)
		msg_write("vulkan destroy");

	if (using_validation_layers)
		destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);

	vkDestroyInstance(instance, nullptr);
}



int parse_version(const string &v) {
	auto vv = v.explode(".");
	if (vv.num >= 2)
		return VK_MAKE_VERSION(vv[0]._int(), vv[1]._int(), 0);
	return VK_MAKE_VERSION(vv[0]._int(), 0, 0);
}

xfer<Instance> Instance::create(const Array<string> &op) {
	string name = "no name";
	int api = VK_API_VERSION_1_0;
	for (auto &o: op) {
		if (o.head(5) == "name=")
			name = o.sub(5);
		if (o.head(4) == "api=")
			api = parse_version(o.sub(4));
		if (o.head(10) == "verbosity=")
			verbosity = o.sub(10)._int();
	}
	if (sa_contains(op, "verbose"))
		verbosity = 10;

	if (verbosity >= 1)
		msg_write("vulkan init");

	Instance *instance = new Instance();

	instance->using_validation_layers = sa_contains(op, "validation");
	if (instance->using_validation_layers and !check_validation_layer_support()) {
		//throw Exception("validation layers requested, but not available!");
		msg_error("validation layers requested, but not available!");
		instance->using_validation_layers = false;
	}

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = name.c_str();
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = name.c_str();
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = api;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
#ifdef OS_MAC
	create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	auto extensions = get_required_instance_extensions(sa_contains(op, "glfw"), instance->using_validation_layers, sa_contains(op, "headless"));
	if (verbosity >= 3)
		for (auto e: extensions)
			msg_write("ENABLING EXTENSION: " + string(e));

	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.num);
	create_info.ppEnabledExtensionNames = &extensions[0];

	Array<const char*> validation_layers;

	if (instance->using_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.num);
		create_info.ppEnabledLayerNames = &validation_layers[0];
	} else {
		create_info.enabledLayerCount = 0;
	}

	auto r = vkCreateInstance(&create_info, nullptr, &instance->instance);
	if (r != VK_SUCCESS)
		throw Exception("failed to create instance! " + result2str(r));


	if (instance->using_validation_layers)
		instance->setup_debug_messenger();

	if (sa_contains(op, "rtx") or sa_contains(op, "rtx?")) {
		try {
			instance->_ensure_rtx_extensions();
		} catch (Exception &e) {
			if (verbosity >= 2)
				msg_write("FAILED TO LOAD RTX EXTENSIONS");
		}
	}
	return instance;
}

#ifdef HAS_LIB_GLFW
VkSurfaceKHR Instance::create_glfw_surface(GLFWwindow* window) {
	VkSurfaceKHR surface;
	auto r = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (r != VK_SUCCESS)
		throw Exception("failed to create window surface!  " + result2str(r));
	return surface;
}
#endif

VkSurfaceKHR Instance::create_headless_surface() {
	VkHeadlessSurfaceCreateInfoEXT info{};
	info.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
	VkSurfaceKHR surface;
	auto r = vkCreateHeadlessSurfaceEXT(instance, &info, nullptr, &surface);
	if (r != VK_SUCCESS)
		throw Exception("failed to create headless surface!  " + result2str(r));
	return surface;
}


#define DECLARE_EXT(NAME) PFN_##NAME _##NAME = nullptr;
#define LOAD_EXT(NAME) \
		_##NAME = (PFN_##NAME)vkGetInstanceProcAddr(instance, #NAME); \
		if (verbosity >= 2) \
			msg_write(format(" %s: %s", #NAME, p2s((void*)_##NAME))); \
		if (!_##NAME) \
			throw Exception("CAN NOT LOAD RTX EXTENSIONS");

DECLARE_EXT(vkCmdTraceRaysKHR);
DECLARE_EXT(vkCmdBuildAccelerationStructuresKHR);
DECLARE_EXT(vkCreateAccelerationStructureKHR);
DECLARE_EXT(vkDestroyAccelerationStructureKHR);
DECLARE_EXT(vkGetAccelerationStructureBuildSizesKHR);
DECLARE_EXT(vkCreateRayTracingPipelinesKHR);
DECLARE_EXT(vkGetAccelerationStructureDeviceAddressKHR);
DECLARE_EXT(vkGetRayTracingShaderGroupHandlesKHR);
DECLARE_EXT(vkGetPhysicalDeviceProperties2);


void Instance::_ensure_rtx_extensions() {
	if (rtx_extensions_loaded)
		return;

	if (verbosity >= 1)
		msg_write("loading rtx extensions...");
	LOAD_EXT(vkCmdTraceRaysKHR);
	LOAD_EXT(vkCmdBuildAccelerationStructuresKHR);
	LOAD_EXT(vkCreateAccelerationStructureKHR);
	LOAD_EXT(vkDestroyAccelerationStructureKHR);
	LOAD_EXT(vkGetAccelerationStructureBuildSizesKHR);
	LOAD_EXT(vkCreateRayTracingPipelinesKHR);
	LOAD_EXT(vkGetAccelerationStructureDeviceAddressKHR);
	LOAD_EXT(vkGetRayTracingShaderGroupHandlesKHR);
	LOAD_EXT(vkGetPhysicalDeviceProperties2);

	if (!_vkCreateRayTracingPipelinesKHR)
		msg_error("CAN NOT LOAD RTX EXTENSIONS");
	if (verbosity >= 3)
		msg_write(" create pipeline: " + p2s((void*)_vkCreateRayTracingPipelinesKHR));

	rtx_extensions_loaded = true;
}

Array<VkLayerProperties> get_available_layers() {
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	Array<VkLayerProperties> available_layers;
	available_layers.resize(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, &available_layers[0]);
	return available_layers;
}

bool check_validation_layer_support() {
	auto available_layers = get_available_layers();

	if (verbosity >= 3) {
		msg_write("available layers:");
		for (const auto& layer_properties : available_layers)
			msg_write(format("  %s", layer_properties.layerName));
	}

	for (const char* layer_name : validation_layers) {
		bool layer_found = false;

		for (const auto& layer_properties : available_layers) {
			if (strcmp(layer_name, layer_properties.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
			return false;
	}

	return true;
}

}

#endif
