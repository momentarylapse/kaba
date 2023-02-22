/*
 * Instance.h
 *
 *  Created on: Sep 21, 2021
 *      Author: michi
 */

#pragma once

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include "../base/pointer.h"

namespace vulkan {

	class Device;

	class Instance {
	public:
		VkInstance instance;
		bool using_validation_layers;
		bool rtx_extensions_loaded = false;

		Instance();
		~Instance();

		VkSurfaceKHR create_surface(GLFWwindow* window);

		void setup_debug_messenger();
		void _ensure_rtx_extensions();

		static xfer<Instance> create(const Array<string> &op);
	};
}

#endif
