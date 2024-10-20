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

	using Surface = VkSurfaceKHR;

	class Device;

	class Instance {
	public:
		VkInstance instance;
		bool using_validation_layers;
		bool rtx_extensions_loaded = false;

		Instance();
		~Instance();

#ifdef HAS_LIB_GLFW
		Surface create_glfw_surface(GLFWwindow* window);
#endif
		Surface create_headless_surface();

		void setup_debug_messenger();
		void _ensure_rtx_extensions();

		static xfer<Instance> create(const Array<string> &op);
	};
}

#endif
