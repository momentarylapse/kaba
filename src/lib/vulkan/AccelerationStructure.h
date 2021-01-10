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

class AccelerationStructure {
public:
	VkAccelerationStructureInfoNV info;
	VkAccelerationStructureNV structure;
	VkDeviceMemory memory;
	uint64_t handle;

	AccelerationStructure(const VkAccelerationStructureTypeNV type, const Array<VkGeometryNV> &geo, const uint32_t instance_count);
	~AccelerationStructure();

	void build(const Array<VkGeometryNV> &geo, const DynamicArray &instances);

	static AccelerationStructure *create_bottom(VertexBuffer *vb);
	static AccelerationStructure *create_top(const DynamicArray &instances);
	static AccelerationStructure *create_top_simple(const Array<AccelerationStructure*> &instances);
};

} /* namespace vulkan */

#endif

