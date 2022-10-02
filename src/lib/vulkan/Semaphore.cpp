/*
 * Semaphore.cpp
 *
 *  Created on: 03.01.2020
 *      Author: michi
 */

#if HAS_LIB_VULKAN


#include "Semaphore.h"
#include "vulkan.h"

namespace vulkan {


Fence::Fence(Device *_device) {
	device = _device->device;
	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(device, &info, nullptr, &fence) != VK_SUCCESS)
		throw Exception("failed to create fence");
}

Fence::~Fence() {
	vkDestroyFence(device, fence, nullptr);
}

void Fence::reset() {
	vkResetFences(device, 1, &fence);
}

void Fence::wait() {
	vkWaitForFences(device, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
}



Semaphore::Semaphore(Device *_device) {
	device = _device->device;
	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device, &info, nullptr, &semaphore) != VK_SUCCESS)
		throw Exception("failed to create semaphore");
}

Semaphore::~Semaphore() {
	vkDestroySemaphore(device, semaphore, nullptr);
}


VkFence fence_handle(Fence *f) {
	if (f)
		return f->fence;
	return VK_NULL_HANDLE;
}

Array<VkSemaphore> extract_semaphores(const Array<Semaphore*> &sem) {
	Array<VkSemaphore> semaphores;
	for (auto *s: sem)
		semaphores.add(s->semaphore);
	return semaphores;
}


} /* namespace vulkan */

#endif

