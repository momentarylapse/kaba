#pragma once

#if HAS_LIB_VULKAN

#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.h>

#ifdef HAS_LIB_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "Instance.h"
#include "Device.h"
#include "Queue.h"
#include "DescriptorSet.h"
#include "Buffer.h"
#include "VertexBuffer.h"
#include "Texture.h"
#include "Shader.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "CommandBuffer.h"
#include "FrameBuffer.h"
#include "SwapChain.h"
#include "Semaphore.h"
#include "AccelerationStructure.h"


namespace vulkan {

	void rebuild_pipelines();

#ifdef HAS_LIB_GLFW
	xfer<GLFWwindow> create_window(const string &title, int width, int height);
	bool window_handle(GLFWwindow *window);
	void window_close(GLFWwindow *window);
#endif

	xfer<Instance> init(const Array<string> &op);


#define DECLARE_EXT_H(NAME) extern PFN_##NAME _##NAME;

	DECLARE_EXT_H(vkCmdTraceRaysNV);
	DECLARE_EXT_H(vkCmdBuildAccelerationStructureNV);
	DECLARE_EXT_H(vkCreateAccelerationStructureNV);
	DECLARE_EXT_H(vkDestroyAccelerationStructureNV);
	DECLARE_EXT_H(vkBindAccelerationStructureMemoryNV);
	DECLARE_EXT_H(vkCreateRayTracingPipelinesNV);
	DECLARE_EXT_H(vkGetAccelerationStructureMemoryRequirementsNV);
	DECLARE_EXT_H(vkGetAccelerationStructureHandleNV);
	DECLARE_EXT_H(vkGetRayTracingShaderGroupHandlesNV);
	DECLARE_EXT_H(vkGetPhysicalDeviceProperties2);
}

#endif
