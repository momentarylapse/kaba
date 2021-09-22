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
#include "helper.h"

#include <iostream>
#include <set>

namespace vulkan {

extern bool verbose;

Device *default_device;

	extern VkSurfaceKHR default_surface;

	bool check_device_extension_support(VkPhysicalDevice device);



	std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	extern Array<const char*> validation_layers;



bool is_device_suitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = find_queue_families(device);
	if (!indices.is_complete())
		return false;

	if (!check_device_extension_support(device))
		return false;

	SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device);
	if ((swapChainSupport.formats.num == 0) or (swapChainSupport.present_modes.num == 0))
		return false;

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(device, &supported_features);
	return supported_features.samplerAnisotropy;
}

bool check_device_extension_support(VkPhysicalDevice device) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	if (verbose)
		std::cout << "---- GPU-----\n";
	for (const auto& extension : available_extensions) {
		if (verbose)
			std::cout << "   " << extension.extensionName << "\n";
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}


Device::Device() {
}

Device::~Device() {
}


void Device::pick_physical_device(Instance *instance) {
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance->instance, &device_count, nullptr);

	if (device_count == 0) {
		throw Exception("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance->instance, &device_count, devices.data());

	if (verbose)
		std::cout << device_count << " devices found\n";

	for (const auto& dev: devices) {
		if (is_device_suitable(dev)) {
			if (verbose)
				std::cout << " ok\n";
			physical_device = dev;
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE) {
		throw Exception("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceProperties(physical_device, &device_properties);
	if (verbose) {
		std::cout << " props\n";
		std::cout << "  minUniformBufferOffsetAlignment  " << device_properties.limits.minUniformBufferOffsetAlignment << "\n";
		std::cout << "  maxPushConstantsSize  " << device_properties.limits.maxPushConstantsSize << "\n";
		std::cout << "  maxImageDimension2D  " << device_properties.limits.maxImageDimension2D << "\n";
		std::cout << "  maxUniformBufferRange  " << device_properties.limits.maxUniformBufferRange << "\n";
		std::cout << "  maxPerStageDescriptorUniformBuffers  " << device_properties.limits.maxPerStageDescriptorUniformBuffers << "\n";
		std::cout << "  maxPerStageDescriptorSamplers  " << device_properties.limits.maxPerStageDescriptorSamplers << "\n";
		std::cout << "  maxDdevice->escriptorSetSamplers  " << device_properties.limits.maxDescriptorSetSamplers << "\n";
		std::cout << "  maxDescriptorSetUniformBuffers  " << device_properties.limits.maxDescriptorSetUniformBuffers << "\n";
		std::cout << "  maxDescriptorSetUniformBuffersDynamic  " << device_properties.limits.maxDescriptorSetUniformBuffersDynamic << "\n";
		//std::cout << "  maxDescriptorSetUniformBuffers  " << device_properties.limits.maxDescriptorSetUniformBuffers << "\n";
		//std::cout << "  maxDescriptorSetUniformBuffers  " << device_properties.limits.maxDescriptorSetUniformBuffers << "\n";
	}

	/*VkPhysicalDeviceRayTracingFeaturesKHR rtf = {};

	VkPhysicalDeviceFeatures2 dp2 = {};
	dp2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	vkGetPhysicalDeviceFeatures2(physical_device, &dp2);*/
	if (verbose)
		std::cout << " done\n";
}

void Device::create_logical_device(bool validation) {
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

	if (validation) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.num);
		create_info.ppEnabledLayerNames = &validation_layers[0];
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
		throw Exception("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue.queue);
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





QueueFamilyIndices find_queue_families(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto& queueFamily: queue_families) {
		if ((queueFamily.queueCount > 0) and (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			indices.graphics_family = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, default_surface, &presentSupport);

		if ((queueFamily.queueCount > 0) and presentSupport) {
			indices.present_family = i;
		}

		if (indices.is_complete()) {
			break;
		}

		i ++;
	}

	return indices;
}


} /* namespace vulkan */

#endif
