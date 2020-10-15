/*
 * AccelerationStructure.cpp
 *
 *  Created on: 12 Oct 2020
 *      Author: michi
 */

#include "AccelerationStructure.h"
#include "helper.h"
#include <iostream>

namespace vulkan {

AccelerationStructure::AccelerationStructure(const VkAccelerationStructureTypeNV type, const Array<VkGeometryNV> &geo, const uint32_t instanceCount) {
	std::cout << format(" + AccStruc  inst=%d  geo=%d\n", instanceCount, geo.num).c_str();
	VkAccelerationStructureCreateInfoNV accelerationStructureInfo = {};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureInfo.compactedSize = 0;
	accelerationStructureInfo.info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.info.type = type;
	accelerationStructureInfo.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
	accelerationStructureInfo.info.instanceCount = instanceCount;
	accelerationStructureInfo.info.geometryCount = geo.num;
	accelerationStructureInfo.info.pGeometries = &geo[0];

	VkResult error = pvkCreateAccelerationStructureNV(device, &accelerationStructureInfo, nullptr, &structure);
	if (VK_SUCCESS != error)
		throw Exception("failed to create acceleration structure");

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.accelerationStructure = structure;

	VkMemoryRequirements2 memoryRequirements;
	pvkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = find_memory_type(memoryRequirements.memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	error = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory);
	if (VK_SUCCESS != error)
		throw Exception("failed to allocate memory");

	VkBindAccelerationStructureMemoryInfoNV bindInfo = {};
	bindInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	bindInfo.accelerationStructure = structure;
	bindInfo.memory = memory;
	bindInfo.memoryOffset = 0;
	bindInfo.deviceIndexCount = 0;
	bindInfo.pDeviceIndices = nullptr;

	error = pvkBindAccelerationStructureMemoryNV(device, 1, &bindInfo);
	if (VK_SUCCESS != error)
		throw Exception("failed to bind acceleration structure");

	error = pvkGetAccelerationStructureHandleNV(device, structure, sizeof(uint64_t), &handle);
	if (VK_SUCCESS != error)
		throw Exception("failed to get acceleration structure handle");

}

AccelerationStructure::~AccelerationStructure() {
	// TODO Auto-generated destructor stub
}

} /* namespace vulkan */
