/*
 * AccelerationStructure.h
 *
 *  Created on: 12 Oct 2020
 *      Author: michi
 */

#ifndef SRC_LIB_VULKAN_ACCELERATIONSTRUCTURE_H_
#define SRC_LIB_VULKAN_ACCELERATIONSTRUCTURE_H_

#include "vulkan.h"

namespace vulkan {

class AccelerationStructure {
public:
	VkAccelerationStructureInfoNV info;
	VkAccelerationStructureKHR structure;
	VkDeviceMemory memory;
	uint64_t handle;

	AccelerationStructure(const VkAccelerationStructureTypeNV type, const Array<VkGeometryNV> &geo, const uint32_t instance_count);
	~AccelerationStructure();

	void build(const Array<VkGeometryNV> &geo, const DynamicArray &instances);

	static AccelerationStructure *create_bottom(VertexBuffer *vb, VertexBuffer *ib);
	static AccelerationStructure *create_top(const DynamicArray &instances);
};

} /* namespace vulkan */

#endif /* SRC_LIB_VULKAN_ACCELERATIONSTRUCTURE_H_ */
