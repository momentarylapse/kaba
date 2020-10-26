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
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	info.type = type;
	info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
	info.instanceCount = instanceCount;
	info.geometryCount = geo.num;
	info.pGeometries = &geo[0];
	VkAccelerationStructureCreateInfoNV ci = {};
	ci.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	ci.compactedSize = 0;
	ci.info = info;

	VkResult error = _vkCreateAccelerationStructureNV(device, &ci, nullptr, &structure);
	if (VK_SUCCESS != error)
		throw Exception("failed to create acceleration structure");

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.accelerationStructure = structure;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;

	VkMemoryRequirements2 memoryRequirements;
	_vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo, &memoryRequirements);

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

	error = _vkBindAccelerationStructureMemoryNV(device, 1, &bindInfo);
	if (VK_SUCCESS != error)
		throw Exception("failed to bind acceleration structure");

	error = _vkGetAccelerationStructureHandleNV(device, structure, sizeof(uint64_t), &handle);
	if (VK_SUCCESS != error)
		throw Exception("failed to get acceleration structure handle");
	std::cout << "handle: " << handle << "\n";

}

AccelerationStructure::~AccelerationStructure() {
    _vkDestroyAccelerationStructureNV(device, structure, nullptr);
    structure = VK_NULL_HANDLE;
    vkFreeMemory(device, memory, nullptr);
    memory = VK_NULL_HANDLE;
}

void AccelerationStructure::build(const Array<VkGeometryNV> &geo, const DynamicArray &instances) {
	std::cout << "   AccStr build\n";

	Buffer instancesBuffer;
	if (instances.num > 0) {
		std::cout << "instance buffer " << instances.num * instances.element_size <<"\n";
		instancesBuffer.create(instances.num * instances.element_size, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		instancesBuffer.update(instances.data);

		info.instanceCount = static_cast<uint32_t>(instances.num);
		info.geometryCount = geo.num;
		info.pGeometries = &geo[0];
	}




	VkAccelerationStructureMemoryRequirementsInfoNV mri;
	mri.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	mri.pNext = nullptr;
	mri.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
	mri.accelerationStructure = structure;

	VkMemoryRequirements2 mem_req = {};
	mem_req.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	_vkGetAccelerationStructureMemoryRequirementsNV(device, &mri, &mem_req);

	Buffer scratch;
	scratch.create(mem_req.memoryRequirements.size, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	auto cb = begin_single_time_commands();

	_vkCmdBuildAccelerationStructureNV(cb, &info,
							instancesBuffer.buffer, 0, VK_FALSE,
							structure, VK_NULL_HANDLE,
							scratch.buffer, 0);

	// multiple needs a memory barrier
	VkMemoryBarrier memoryBarrier;
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);
	end_single_time_commands(cb);

}

} /* namespace vulkan */
