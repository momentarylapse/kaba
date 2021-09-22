/*
 * Instance.h
 *
 *  Created on: Sep 21, 2021
 *      Author: michi
 */

#pragma once

#if HAS_LIB_VULKAN

#include "../base/base.h"

namespace vulkan {

	class Device;

	class Instance {
	public:
		VkInstance instance;
		bool using_validation_layers;

		Instance();
		~Instance();

		void __delete__();

		Device *pick_device();
		VkSurfaceKHR create_surface(GLFWwindow* window);

		void setup_debug_messenger();
		void _ensure_rtx();

		static Instance *create(const Array<string> &op);
	};
}

#endif
