/*
 * AccelerationStructure.cpp
 *
 *  Created on: 12 Oct 2020
 *      Author: michi
 */

#if HAS_LIB_VULKAN

#include "AccelerationStructure.h"
#include "helper.h"
#include "../math/mat4.h"
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

	VkResult error = _vkCreateAccelerationStructureNV(default_device->device, &ci, nullptr, &structure);
	if (VK_SUCCESS != error)
		throw Exception("failed to create acceleration structure");

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.accelerationStructure = structure;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;

	VkMemoryRequirements2 memoryRequirements;
	_vkGetAccelerationStructureMemoryRequirementsNV(default_device->device, &memoryRequirementsInfo, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = default_device->find_memory_type(memoryRequirements.memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	error = vkAllocateMemory(default_device->device, &memoryAllocateInfo, nullptr, &memory);
	if (VK_SUCCESS != error)
		throw Exception("failed to allocate memory");

	VkBindAccelerationStructureMemoryInfoNV bindInfo = {};
	bindInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	bindInfo.accelerationStructure = structure;
	bindInfo.memory = memory;
	bindInfo.memoryOffset = 0;
	bindInfo.deviceIndexCount = 0;
	bindInfo.pDeviceIndices = nullptr;

	error = _vkBindAccelerationStructureMemoryNV(default_device->device, 1, &bindInfo);
	if (VK_SUCCESS != error)
		throw Exception("failed to bind acceleration structure");

	error = _vkGetAccelerationStructureHandleNV(default_device->device, structure, sizeof(uint64_t), &handle);
	if (VK_SUCCESS != error)
		throw Exception("failed to get acceleration structure handle");
	std::cout << "handle: " << handle << "\n";

}

AccelerationStructure::~AccelerationStructure() {
	_vkDestroyAccelerationStructureNV(default_device->device, structure, nullptr);
	structure = VK_NULL_HANDLE;
	vkFreeMemory(default_device->device, memory, nullptr);
	memory = VK_NULL_HANDLE;
}

void AccelerationStructure::build(const Array<VkGeometryNV> &geo, const DynamicArray &instances) {
	std::cout << "   AccStr build\n";

	Buffer instancesBuffer;
	if (instances.num > 0) {
		std::cout << p2s(&instances).c_str() << "\n";
		std::cout << "instance buffer " << instances.num <<"*"<< instances.element_size <<"\n";
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
	_vkGetAccelerationStructureMemoryRequirementsNV(default_device->device, &mri, &mem_req);

	Buffer scratch;
	scratch.create(mem_req.memoryRequirements.size, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	auto cb = begin_single_time_commands();

	_vkCmdBuildAccelerationStructureNV(cb->buffer, &info,
							instancesBuffer.buffer, 0, VK_FALSE,
							structure, VK_NULL_HANDLE,
							scratch.buffer, 0);

	// multiple needs a memory barrier
	VkMemoryBarrier memoryBarrier;
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier(cb->buffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);
	end_single_time_commands(cb);

}

AccelerationStructure *AccelerationStructure::create_bottom(VertexBuffer *vb) {
	Array<VkGeometryNV> geo;

	//for (int i=0; i<vb.num; i++) {
		VkGeometryNV geometry = {};

		geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		geometry.geometry.triangles.vertexData = vb->vertex_buffer.buffer;
		geometry.geometry.triangles.vertexOffset = 0;
		geometry.geometry.triangles.vertexCount = vb->vertex_count;
		geometry.geometry.triangles.vertexStride = sizeof(float)*3;
		geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometry.geometry.triangles.indexData = VK_NULL_HANDLE;
		//geometry.geometry.triangles.indexData = vb->index_buffer.buffer;
		geometry.geometry.triangles.indexOffset = 0;
		geometry.geometry.triangles.indexCount = vb->output_count;
		geometry.geometry.triangles.indexType = vb->index_type;
		geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
		geometry.geometry.triangles.transformOffset = 0;
		geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;
		geo.add(geometry);
		std::cout << vb->output_count << "\n";
	//}

	AccelerationStructure *as = new AccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV, geo, 0);
	as->build(geo, {});
	return as;
}

AccelerationStructure *AccelerationStructure::create_top(const DynamicArray &instances) {
	std::cout << p2s(&instances).c_str() << "  ct\n";
	AccelerationStructure *as = new AccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV, {}, instances.num);
	as->build({}, instances);
	return as;
}


AccelerationStructure *AccelerationStructure::create_top_simple(const Array<AccelerationStructure*> &blas) {

	struct VkGeometryInstance {
	    float transform[12];
	    uint32_t instanceId : 24;
	    uint32_t mask : 8;
	    uint32_t instanceOffset : 24;
	    uint32_t flags : 8;
	    uint64_t accelerationStructureHandle;
	};

    Array<VkGeometryInstance> instances;
    instances.resize(blas.num);

    for (size_t i = 0; i < blas.num; ++i) {

        VkGeometryInstance& instance = instances[i];
        memcpy(instance.transform, &mat4::ID, 12*4);
        instance.instanceId = static_cast<uint32_t>(i);
        instance.mask = 0xff;
        instance.instanceOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
        instance.accelerationStructureHandle = blas[i]->handle;
    }


    return create_top(instances);
}

} /* namespace vulkan */

#endif

