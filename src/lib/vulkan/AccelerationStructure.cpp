/*
 * AccelerationStructure.cpp
 *
 *  Created on: 12 Oct 2020
 *      Author: michi
 */

#if HAS_LIB_VULKAN

#include "AccelerationStructure.h"
#include "helper.h"
#include "common.h"
#include "../math/mat4.h"
#include "../os/msg.h"

namespace vulkan {

AccelerationStructure::AccelerationStructure(Device* _device) : buffer(_device) {
	device = _device;
	info = {};
	structure = VK_NULL_HANDLE;
	handle = 0;
}

#if 0
AccelerationStructure::AccelerationStructure(Device* _device, const VkAccelerationStructureTypeKHR type, const Array<VkAccelerationStructureGeometryKHR>& geo, const uint32_t instanceCount) {
	if (verbosity >= 2)
		msg_write(format(" + AccStruc  inst=%d  geo=%d", instanceCount, geo.num));
	device = _device;

	info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	info.type = type;
	info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	info.geometryCount = geo.num;
	info.pGeometries = &geo[0];
	// TODO top layer...
	info.instanceCount = instanceCount;


	const uint32_t numTriangles = 1;
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	_vkGetAccelerationStructureBuildSizesKHR(
		device->device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&info,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);

	createAccelerationStructureBuffer(this, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR ci = {};
	ci.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	ci.buffer = buffer.buffer;
	ci.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	ci.type = type;

	VkResult error = _vkCreateAccelerationStructureKHR(device->device, &ci, nullptr, &structure);
	if (VK_SUCCESS != error)
		throw Exception("failed to create acceleration structure");


	// ----------------------------------------------


	VkAccelerationStructureMemoryRequirementsInfoNV memory_requirements_info = {};
	memory_requirements_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memory_requirements_info.accelerationStructure = structure;
	memory_requirements_info.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;

	VkMemoryRequirements2 memoryRequirements;
	_vkGetAccelerationStructureMemoryRequirementsNV(device->device, &memory_requirements_info, &memoryRequirements);

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memoryRequirements.memoryRequirements.size;
	memory_allocate_info.memoryTypeIndex = device->find_memory_type(memoryRequirements.memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	error = vkAllocateMemory(device->device, &memory_allocate_info, nullptr, &memory);
	if (VK_SUCCESS != error)
		throw Exception("failed to allocate memory");

	VkBindAccelerationStructureMemoryInfoNV bind_info = {};
	bind_info.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	bind_info.accelerationStructure = structure;
	bind_info.memory = memory;
	bind_info.memoryOffset = 0;
	bind_info.deviceIndexCount = 0;
	bind_info.pDeviceIndices = nullptr;

	error = _vkBindAccelerationStructureMemoryNV(device, 1, &bind_info);
	if (VK_SUCCESS != error)
		throw Exception("failed to bind acceleration structure");

	error = _vkGetAccelerationStructureHandleNV(device, structure, sizeof(uint64_t), &handle);
	if (VK_SUCCESS != error)
		throw Exception("failed to get acceleration structure handle");
	if (verbosity >= 2)
		msg_write("handle: " + i2s(handle));
}
#endif

AccelerationStructure::~AccelerationStructure() {
	_vkDestroyAccelerationStructureKHR(device->device, structure, nullptr);
	//buffer.destroy();
}

#if 0
void AccelerationStructure::build(const Array<VkAccelerationStructureGeometryKHR>& geo, const Array<VkAccelerationStructureInstanceKHR>& instances, bool update) {
	if (verbosity >= 4)
		msg_write("   AccStr build");

	Buffer instances_buffer(default_device);
	if (instances.num > 0) {
		if (verbosity >= 4) {
			msg_write(p2s(&instances));
			msg_write(format("instance buffer %d*%d", instances.num, instances.element_size));
		}
		instances_buffer.create(instances.num * instances.element_size, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		instances_buffer.update(instances.data);

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

	Buffer scratch(default_device);
	scratch.create(mem_req.memoryRequirements.size, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	auto cb = begin_single_time_commands();

	_vkCmdBuildAccelerationStructuresKHR(cb->buffer, &info,
							instances_buffer.buffer, 0, update, //VK_FALSE,
							structure, VK_NULL_HANDLE,
							scratch.buffer, 0);

	// multiple needs a memory barrier
	VkMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	vkCmdPipelineBarrier(cb->buffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &barrier, 0, 0, 0, 0);
	end_single_time_commands(cb);
}
#endif

static Array<VkAccelerationStructureGeometryKHR> create_geometries(VertexBuffer* vb) {
	Array<VkAccelerationStructureGeometryKHR> geo;

	//for (int i=0; i<vb.num; i++) {
		if (verbosity >= 4)
			msg_write(format("AS vertices=%d stride=%d", vb->vertex_count, vb->stride()));
		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometry.geometry.triangles.vertexData.deviceAddress = vb->vertex_buffer.get_device_address();
		geometry.geometry.triangles.maxVertex = vb->vertex_count - 1;
		geometry.geometry.triangles.vertexStride = vb->stride();
		geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		if (vb->is_indexed()) {
			if (verbosity >= 3)
				msg_write("AS indexed");
			geometry.geometry.triangles.indexData.deviceAddress = vb->index_buffer.get_device_address();
//			geometry.geometry.triangles.indexCount = vb->output_count;
			geometry.geometry.triangles.indexType = vb->index_type;
		}
		geometry.geometry.triangles.transformData = {};
		//geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		geo.add(geometry);
		if (verbosity >= 3)
			msg_write(vb->output_count);
	//}
	return geo;
}

static Array<VkAccelerationStructureInstanceKHR> create_instances(const Array<AccelerationStructure*> &blas, const Array<mat4> &matrices) {
	Array<VkAccelerationStructureInstanceKHR> instances;
	instances.resize(blas.num);

	int triangle_offset = 0;
	for (int i = 0; i < blas.num; i++) {
		auto &instance = instances[i];
		memcpy(&instance.transform, &matrices[i], 12*sizeof(float));
		instance.instanceCustomIndex = static_cast<uint32_t>(triangle_offset);
		instance.mask = 0xff;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference = blas[i]->handle;
		triangle_offset += blas[i]->triangle_count;
	}
	return instances;
}

void AccelerationStructure::update_top(const Array<AccelerationStructure*>& blas, const Array<mat4>& matrices) {
	//auto instances = create_instances(blas, matrices);
	//build({}, instances, true);
}


AccelerationStructure* AccelerationStructure::create_bottom(Device* device, VertexBuffer* vb) {
	msg_write("  ACC STRUC BOTTOM...");
	auto as = new AccelerationStructure(device);
	as->_create_bottom(vb);
	return as;
}

void AccelerationStructure::create_buffer(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo) {
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	if (vkCreateBuffer(device->device, &bufferCreateInfo, nullptr, &buffer.buffer) != VK_SUCCESS)
		throw Exception("failed to create buffer");
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(device->device, buffer.buffer, &memoryRequirements);
	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
	memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = device->find_memory_type(memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(device->device, &memoryAllocateInfo, nullptr, &memory) != VK_SUCCESS)
		throw Exception("failed to allocate memory");
	if (vkBindBufferMemory(device->device, buffer.buffer, memory, 0) != VK_SUCCESS)
		throw Exception("failed to bind buffer memory");
}

void AccelerationStructure::_create_bottom(VertexBuffer *vb) {
	auto geometries = create_geometries(vb);
	triangle_count = vb->output_count / 3;

	auto type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	info.type = type;
	info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	info.geometryCount = geometries.num;
	info.pGeometries = &geometries[0];


	const uint32_t numTriangles = triangle_count;
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	_vkGetAccelerationStructureBuildSizesKHR(
		device->device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&info,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);

	create_buffer(accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR ci = {};
	ci.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	ci.buffer = buffer.buffer;
	ci.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	ci.type = type;

	VkResult error = _vkCreateAccelerationStructureKHR(device->device, &ci, nullptr, &structure);
	if (VK_SUCCESS != error)
		throw Exception("failed to create acceleration structure");


	Buffer scratch(default_device);
	scratch.create(accelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	// Create a small scratch buffer used during build of the bottom level acceleration structure
//	RayTracingScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

	info.scratchData.deviceAddress = scratch.get_device_address();
	info.dstAccelerationStructure = structure;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	//VkCommandBuffer commandBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	auto cb = begin_single_time_commands();
	_vkCmdBuildAccelerationStructuresKHR(
		cb->buffer,
		1,
		&info,
		accelerationBuildStructureRangeInfos.data());
	//vulkanDevice->flushCommandBuffer(cb->buffer, queue);

	// multiple needs a memory barrier
	VkMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
//	vkCmdPipelineBarrier(cb->buffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &barrier, 0, 0, 0, 0);
	end_single_time_commands(cb);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = structure;
	handle = _vkGetAccelerationStructureDeviceAddressKHR(device->device, &accelerationDeviceAddressInfo);
}


void AccelerationStructure::_create_top(const Array<AccelerationStructure *> &blas, const Array<mat4> &matrices) {
	auto instances = create_instances(blas, matrices);

	Buffer instance_buffer(device);
	instance_buffer.create(sizeof(VkAccelerationStructureInstanceKHR) * instances.num,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	instance_buffer.update_array(instances);

	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	instanceDataDeviceAddress.deviceAddress = instance_buffer.get_device_address();

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	// Get size info
	/*
	The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored. Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that the hostAddress member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if it is NULL.*
	*/
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	uint32_t primitive_count = 1;

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	_vkGetAccelerationStructureBuildSizesKHR(
		device->device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&primitive_count,
		&accelerationStructureBuildSizesInfo);

	create_buffer(accelerationStructureBuildSizesInfo);
	//createAccelerationStructureBuffer(topLevelAS, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = buffer.buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	_vkCreateAccelerationStructureKHR(device->device, &accelerationStructureCreateInfo, nullptr, &structure);

	msg_write("=> " + p2s(structure));

	Buffer scratch(default_device);
	scratch.create(accelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	info.dstAccelerationStructure = structure;
	info.geometryCount = 1;
	info.pGeometries = &accelerationStructureGeometry;
	info.scratchData.deviceAddress = scratch.get_device_address();

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = 1;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };


	auto cb = begin_single_time_commands();
	_vkCmdBuildAccelerationStructuresKHR(
		cb->buffer,
		1,
		&info,
		accelerationBuildStructureRangeInfos.data());
	end_single_time_commands(cb);


	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = structure;
	handle = _vkGetAccelerationStructureDeviceAddressKHR(device->device, &accelerationDeviceAddressInfo);

//	scratch.destroy();
//	instance_buffer.destroy();
}


AccelerationStructure* AccelerationStructure::create_top(Device* device, const Array<AccelerationStructure*>& blas, const Array<mat4>& matrices) {
	msg_write("  ACC STRUC TOP...");
	auto as = new AccelerationStructure(device);
	as->_create_top(blas, matrices);
	//as->build({}, instances, false);
	return as;
}

} /* namespace vulkan */

#endif

