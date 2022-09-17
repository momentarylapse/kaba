/*
 * Device.cpp
 *
 *  Created on: Oct 27, 2020
 *      Author: michi
 */

#if HAS_LIB_VULKAN

#include <vulkan/vulkan.h>
#include "Device.h"
#include "vulkan.h"
#include "Queue.h"
#include "helper.h"
#include "common.h"

#include <iostream>
#include "../base/set.h"
#include "../os/msg.h"

namespace vulkan {

extern bool verbose;

Device *default_device;

	bool check_device_extension_support(VkPhysicalDevice device, Requirements req);



	Array<const char*> device_extensions(Requirements req) {
		Array<const char*> ext;
		if (req & Requirements::SWAP_CHAIN)
			ext.add(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		if (req & Requirements::RTX) {
			ext.add(VK_NV_RAY_TRACING_EXTENSION_NAME);
			ext.add(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
		}
		return ext;
	}

	extern Array<const char*> validation_layers;


bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface, Requirements req) {
	auto indices = QueueFamilyIndices::query(device, surface, req);
	if (!indices)
		return false;

	if (!check_device_extension_support(device, req))
		return false;

	SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device, surface);

	if (req & Requirements::SWAP_CHAIN)
		if (swapChainSupport.formats.num == 0)
			return false;

	if (req & Requirements::PRESENT)
		if (swapChainSupport.present_modes.num == 0)
			return false;

	if (req & Requirements::ANISOTROPY) {
		VkPhysicalDeviceFeatures supported_features;
		vkGetPhysicalDeviceFeatures(device, &supported_features);
		if (!supported_features.samplerAnisotropy)
			return false;
	}

	return true;
}

bool check_device_extension_support(VkPhysicalDevice device, Requirements req) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	Array<VkExtensionProperties> available_extensions;
	available_extensions.resize(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, &available_extensions[0]);

	base::set<string> required_extensions;
	for (auto e: device_extensions(req))
		required_extensions.add(e);

	if (verbose)
		std::cout << "---- GPU-----\n";
	for (const auto& extension : available_extensions) {
		if (verbose)
			std::cout << "   " << extension.extensionName << "\n";
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.num == 0;
}


Device::Device() {
}

Device::~Device() {
	if (surface)
		vkDestroySurfaceKHR(instance->instance, surface, nullptr);
	if (device)
		vkDestroyDevice(device, nullptr);
}


void Device::pick_physical_device(Instance *_instance, VkSurfaceKHR _surface, Requirements req) {
	instance = _instance;
	surface = _surface;

	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance->instance, &device_count, nullptr);

	if (device_count == 0)
		throw Exception("failed to find GPUs with Vulkan support!");

	Array<VkPhysicalDevice> devices;
	devices.resize(device_count);
	vkEnumeratePhysicalDevices(instance->instance, &device_count, &devices[0]);

	if (verbose)
		std::cout << device_count << " devices found\n";

	physical_device = VK_NULL_HANDLE;
	for (const auto& dev: devices) {
		if (is_device_suitable(dev, surface, req)) {
			if (verbose)
				std::cout << " ok\n";
			physical_device = dev;
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE)
		throw Exception("failed to find a suitable GPU!");

	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	if (verbose) {
		std::cout << " props\n";
		std::cout << "  minUniformBufferOffsetAlignment  " << physical_device_properties.limits.minUniformBufferOffsetAlignment << "\n";
		std::cout << "  maxPushConstantsSize  " << physical_device_properties.limits.maxPushConstantsSize << "\n";
		std::cout << "  maxImageDimension2D  " << physical_device_properties.limits.maxImageDimension2D << "\n";
		std::cout << "  maxUniformBufferRange  " << physical_device_properties.limits.maxUniformBufferRange << "\n";
		std::cout << "  maxPerStageDescriptorUniformBuffers  " << physical_device_properties.limits.maxPerStageDescriptorUniformBuffers << "\n";
		std::cout << "  maxPerStageDescriptorSamplers  " << physical_device_properties.limits.maxPerStageDescriptorSamplers << "\n";
		std::cout << "  maxDdevice->escriptorSetSamplers  " << physical_device_properties.limits.maxDescriptorSetSamplers << "\n";
		std::cout << "  maxDescriptorSetUniformBuffers  " << physical_device_properties.limits.maxDescriptorSetUniformBuffers << "\n";
		std::cout << "  maxDescriptorSetUniformBuffersDynamic  " << physical_device_properties.limits.maxDescriptorSetUniformBuffersDynamic << "\n";
		//std::cout << "  maxDescriptorSetUniformBuffers  " << physical_device_properties.limits.maxDescriptorSetUniformBuffers << "\n";
		//std::cout << "  maxDescriptorSetUniformBuffers  " << physical_device_properties.limits.maxDescriptorSetUniformBuffers << "\n";
	}

	/*VkPhysicalDeviceRayTracingFeaturesKHR rtf = {};

	VkPhysicalDeviceFeatures2 dp2 = {};
	dp2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	vkGetPhysicalDeviceFeatures2(physical_device, &dp2);*/
	if (verbose)
		std::cout << " done\n";
}

void Device::create_logical_device(VkSurfaceKHR surface, Requirements req) {
	indices = *QueueFamilyIndices::query(physical_device, surface, req);

	Array<VkDeviceQueueCreateInfo> queue_create_infos;
	auto unique_queue_families = indices.unique();

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

	auto extensions = device_extensions(req);
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.num);
	create_info.ppEnabledExtensionNames = &extensions[0];

	if (req & Requirements::VALIDATION) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.num);
		create_info.ppEnabledLayerNames = &validation_layers[0];
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
		throw Exception("failed to create logical device!");

	if (indices.graphics_family)
		vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue.queue);
	if (indices.present_family)
		vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue.queue);
}



void Device::wait_idle() {
	vkDeviceWaitIdle(device);
}



uint32_t Device::find_memory_type(const VkMemoryRequirements &requirements, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

	for (uint32_t i=0; i<memProperties.memoryTypeCount; i++) {
		if ((requirements.memoryTypeBits & (1 << i)) and (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw Exception("failed to find suitable memory type!");
}


VkFormat Device::find_supported_format(const Array<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format: candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR and (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL and (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw Exception("failed to find supported format!");
}

VkFormat Device::find_depth_format() {
	return find_supported_format(
	{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}





void Device::create_query_pool(int count) {
	VkQueryPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.queryType = VK_QUERY_TYPE_TIMESTAMP;
	info.queryCount = count;
	vkCreateQueryPool(device, &info, nullptr, &query_pool);
}

void Device::reset_query_pool(int first, int count) {
	vkResetQueryPool(device, query_pool, first, count);
}

Array<int> Device::get_timestamps(int first, int count) {
	Array<int> tt;
	tt.resize(count);
	vkGetQueryPoolResults(device, query_pool, first, count, sizeof(tt[0]) * tt.num, &tt[0], 4, VK_QUERY_RESULT_PARTIAL_BIT);//VK_QUERY_RESULT_WAIT_BIT);
	return tt;
}


int Device::make_aligned(int size) {
	if (physical_device_properties.limits.minUniformBufferOffsetAlignment == 0)
		return 0;
	return (size + physical_device_properties.limits.minUniformBufferOffsetAlignment - 1) & ~(size - 1);
}

Requirements parse_requirements(const Array<string> &op) {
	Requirements req = Requirements::NONE;
	for (auto &o: op) {
		if (o == "validation")
			req = req | Requirements::VALIDATION;
		else if (o == "graphics")
			req = req | Requirements::GRAPHICS;
		else if (o == "present")
			req = req | Requirements::PRESENT;
		else if (o == "compute")
			req = req | Requirements::COMPUTE;
		else if (o == "swapchain")
			req = req | Requirements::SWAP_CHAIN;
		else if (o == "anisotropy")
			req = req | Requirements::ANISOTROPY;
		else if (o == "rtx")
			req = req | Requirements::RTX;
		else
			throw Exception("unknown requirement: " + o);
	}
	return req;
}

Device *Device::create_simple(Instance *instance, GLFWwindow* window, const Array<string> &op) {
	auto surface = instance->create_surface(window);

	//op.append({"graphics", "present", "swapchain", "anisotropy"});
	auto req = parse_requirements(op);
	auto device = new Device();
	device->pick_physical_device(instance, surface, req);
	device->create_logical_device(surface, req);

	create_command_pool(device);

	if (sa_contains(op, "rtx"))
		device->get_rtx_properties();

	default_device = device;
	return device;
}



void Device::get_rtx_properties() {

	ray_tracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

	VkPhysicalDeviceProperties2 dev_props;
	dev_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	dev_props.pNext = &ray_tracing_properties;
	dev_props.properties = { };

	//pvkGetPhysicalDeviceProperties2() FIXME
	vkGetPhysicalDeviceProperties2(physical_device, &dev_props);
	if (verbose) {
		msg_write("PROPS");
		msg_write(ray_tracing_properties.maxShaderGroupStride);
		msg_write(ray_tracing_properties.shaderGroupBaseAlignment);
		msg_write(ray_tracing_properties.shaderGroupHandleSize);
	}
}



} /* namespace vulkan */

#endif
