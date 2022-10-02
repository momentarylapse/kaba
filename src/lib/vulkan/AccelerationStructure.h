/*
 * AccelerationStructure.h
 *
 *  Created on: 12 Oct 2020
 *      Author: michi
 */

#pragma once

#if HAS_LIB_VULKAN

#include "vulkan.h"

namespace vulkan {

	class Device;


class AccelerationStructure {
public:
	VkAccelerationStructureInfoNV info;
	VkAccelerationStructureNV structure;
	VkDeviceMemory memory;
	uint64_t handle;
	VkDevice device;

	AccelerationStructure(Device *device, const VkAccelerationStructureTypeNV type, const Array<VkGeometryNV> &geo, const uint32_t instance_count);
	~AccelerationStructure();

	void build(const Array<VkGeometryNV> &geo, const DynamicArray &instances);

	static AccelerationStructure *create_bottom(Device *device, VertexBuffer *vb);
	static AccelerationStructure *create_top(Device *device, const DynamicArray &instances);
	static AccelerationStructure *create_top_simple(Device *device, const Array<AccelerationStructure*> &instances);
};

} /* namespace vulkan */

#endif

