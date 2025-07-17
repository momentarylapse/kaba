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
	VkAccelerationStructureBuildGeometryInfoKHR info;
	VkAccelerationStructureKHR structure;
	Buffer buffer;
	VkDeviceMemory memory;
	uint64_t handle;
	Device* device;
	int triangle_count = 0;

	//AccelerationStructure(Device* device, VkAccelerationStructureTypeKHR type, const Array<VkAccelerationStructureGeometryKHR>& geo, const uint32_t instance_count);
	explicit AccelerationStructure(Device* device);
	~AccelerationStructure();

	void _create_top(const Array<AccelerationStructure*>& blas, const Array<mat4>& matrices);
	void _create_bottom(VertexBuffer* vb);

	void create_buffer(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

	void update_top(const Array<AccelerationStructure*>& blas, const Array<mat4>& matrices);

	static AccelerationStructure* create_bottom(Device* device, VertexBuffer* vb);
	static AccelerationStructure* create_top(Device* device, const Array<AccelerationStructure*>& blas, const Array<mat4>& matrices);

private:
	//void build(const Array<VkAccelerationStructureGeometryKHR>& geo, const Array<VkAccelerationStructureInstanceKHR>& instances, bool update);
};

} /* namespace vulkan */

#endif

