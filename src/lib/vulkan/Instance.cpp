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


Array<const char*> get_required_instance_extensions(bool glfw, bool validation) {
	Array<const char*> extensions;

	// default
	extensions.add(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	if (glfw) {
		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		for (uint32_t i=0; i<glfw_extension_count; i++)
			extensions.add(glfw_extensions[i]);
	}

	if (validation)
		extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
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

		if (create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS)
			throw Exception("failed to set up debug messenger!");
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

	auto extensions = get_required_instance_extensions(sa_contains(op, "glfw"), instance->using_validation_layers);
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.num);
	create_info.ppEnabledExtensionNames = &extensions[0];

	Array<const char*> validation_layers;

	if (instance->using_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.num);
		create_info.ppEnabledLayerNames = &validation_layers[0];
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&create_info, nullptr, &instance->instance) != VK_SUCCESS)
		throw Exception("failed to create instance!");


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

VkSurfaceKHR Instance::create_surface(GLFWwindow* window) {
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) 
		throw Exception("failed to create window surface!");
	return surface;
}



#define DECLARE_EXT(NAME) PFN_##NAME _##NAME = nullptr;
#define LOAD_EXT(NAME) \
		_##NAME = (PFN_##NAME)vkGetInstanceProcAddr(instance, #NAME); \
		if (verbosity >= 2) \
			msg_write(format(" %s: %s", #NAME, p2s((void*)_##NAME))); \
		if (!_##NAME) \
			throw Exception("CAN NOT LOAD RTX EXTENSIONS");

DECLARE_EXT(vkCmdTraceRaysNV);
DECLARE_EXT(vkCreateRayTracingPipelinesNV);
DECLARE_EXT(vkCmdBuildAccelerationStructureNV);
DECLARE_EXT(vkCreateAccelerationStructureNV);
DECLARE_EXT(vkDestroyAccelerationStructureNV);
DECLARE_EXT(vkBindAccelerationStructureMemoryNV);
DECLARE_EXT(vkGetAccelerationStructureMemoryRequirementsNV);
DECLARE_EXT(vkGetAccelerationStructureHandleNV);
DECLARE_EXT(vkGetRayTracingShaderGroupHandlesNV);
DECLARE_EXT(vkGetPhysicalDeviceProperties2);


void Instance::_ensure_rtx_extensions() {
	if (rtx_extensions_loaded)
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

	if (verbosity >= 1)
		msg_write("loading rtx extensions...");
	LOAD_EXT(vkCmdTraceRaysNV);
	LOAD_EXT(vkCreateRayTracingPipelinesNV);
	LOAD_EXT(vkCmdBuildAccelerationStructureNV);
	LOAD_EXT(vkCreateAccelerationStructureNV);
	LOAD_EXT(vkDestroyAccelerationStructureNV);
	LOAD_EXT(vkBindAccelerationStructureMemoryNV);
	LOAD_EXT(vkGetAccelerationStructureMemoryRequirementsNV);
	LOAD_EXT(vkGetAccelerationStructureHandleNV);
	LOAD_EXT(vkGetRayTracingShaderGroupHandlesNV);
	LOAD_EXT(vkGetPhysicalDeviceProperties2);

	if (!_vkCreateRayTracingPipelinesNV)
		msg_error("CAN NOT LOAD RTX EXTENSIONS");
	if (verbosity >= 3)
		msg_write(" create pipeline: " + p2s((void*)_vkCreateRayTracingPipelinesNV));

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
