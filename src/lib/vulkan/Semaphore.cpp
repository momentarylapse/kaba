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


Fence::Fence() {
	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(default_device->device, &info, nullptr, &fence) != VK_SUCCESS) {
		throw Exception("failed to create fence");
	}
}

Fence::~Fence() {
	vkDestroyFence(default_device->device, fence, nullptr);
}

void Fence::__init__() {
	new(this) Fence;
}

void Fence::__delete__() {
	this->~Fence();
}

void Fence::reset() {
	vkResetFences(default_device->device, 1, &fence);
}

void Fence::wait() {
	vkWaitForFences(default_device->device, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
}



Semaphore::Semaphore() {
	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(default_device->device, &info, nullptr, &semaphore) != VK_SUCCESS) {
		throw Exception("failed to create semaphore");
	}
}

Semaphore::~Semaphore() {
	vkDestroySemaphore(default_device->device, semaphore, nullptr);
}

void Semaphore::__init__() {
	new(this) Semaphore;
}

void Semaphore::__delete__() {
	this->~Semaphore();
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

