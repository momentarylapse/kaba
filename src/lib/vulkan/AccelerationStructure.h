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
	int triangle_count = 0;

	AccelerationStructure(Device *device, const VkAccelerationStructureTypeNV type, const Array<VkGeometryNV> &geo, const uint32_t instance_count);
	~AccelerationStructure();

	void update_top(const Array<AccelerationStructure*> &blas, const Array<mat4> &matrices);

	static AccelerationStructure *create_bottom(Device *device, VertexBuffer *vb);
	static AccelerationStructure *create_top(Device *device, const Array<AccelerationStructure*> &blas, const Array<mat4> &matrices);

private:
	void build(const Array<VkGeometryNV> &geo, const Array<VkAccelerationStructureInstanceKHR> &instances, bool update);
};

} /* namespace vulkan */

#endif

