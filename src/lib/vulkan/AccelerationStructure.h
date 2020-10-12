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
	VkAccelerationStructureKHR structure;
	VkDeviceMemory memory;
	uint64_t handle;

	AccelerationStructure(const VkAccelerationStructureTypeNV type, const Array<VkGeometryNV> &geo, const uint32_t instance_count);
	~AccelerationStructure();
};

} /* namespace vulkan */

#endif /* SRC_LIB_VULKAN_ACCELERATIONSTRUCTURE_H_ */
