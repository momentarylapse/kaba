/*
 * Device.h
 *
 *  Created on: Oct 27, 2020
 *      Author: michi
 */

#pragma once

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include "Queue.h"

namespace vulkan {

	class Instance;


class Device {
public:
	Device();
	~Device();

	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device;

	Queue graphics_queue;
	Queue present_queue;

	VkPhysicalDeviceProperties device_properties;



	void wait_idle();


	void pick_physical_device(Instance *instance);
	void create_logical_device(bool validation);


	uint32_t find_memory_type(const VkMemoryRequirements &requirements, VkMemoryPropertyFlags properties);
	VkFormat find_supported_format(const Array<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat find_depth_format();

	VkQueryPool query_pool;
	void create_query_pool(int count);
	void reset_query_pool(int first, int count);
	Array<int> get_timestamps(int first, int count);
};

extern Device *default_device;

QueueFamilyIndices find_queue_families(VkPhysicalDevice device);


} /* namespace vulkan */

#endif
