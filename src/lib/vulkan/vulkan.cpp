#if HAS_LIB_VULKAN

#include "vulkan.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "helper.h"
#include "../base/base.h"
#include "../os/msg.h"

//#define NDEBUG

Array<const char*> sa2pa(const Array<string> &sa) {
	Array<const char*> pa;
	for (string &s: sa)
		pa.add((const char*)s.data);
	return pa;
}




namespace vulkan {

bool verbose = true;

extern VkSurfaceKHR default_surface;
VkQueryPool query_pool;




Instance *init(GLFWwindow* window, const Array<string> &op) {
	msg_write("vulkan init");

	bool want_rtx = sa_contains(op, "rtx");

	auto instance = Instance::create(op);
	default_surface = instance->create_surface(window);
	default_device = instance->pick_device();
	create_command_pool();
	default_device->create_query_pool(16384);

	if (want_rtx)
		vulkan::rtx::get_properties();

	return instance;
}






GLFWwindow* create_window(const string &title, int width, int height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	//glfwSetWindowUserPointer(window, this);
	return window;
}

bool window_handle(GLFWwindow *window) {
	if (glfwWindowShouldClose(window))
		return true;
	glfwPollEvents();
	return false;
}

void window_close(GLFWwindow *window) {
	glfwDestroyWindow(window);

	glfwTerminate();
}






namespace rtx {

VkPhysicalDeviceRayTracingPropertiesNV properties;

void get_properties() {

	properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

	VkPhysicalDeviceProperties2 devProps;
	devProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	devProps.pNext = &properties;
	devProps.properties = { };

	//pvkGetPhysicalDeviceProperties2() FIXME
	vkGetPhysicalDeviceProperties2(default_device->physical_device, &devProps);
	if (verbose) {
		msg_write("PROPS");
		msg_write(properties.maxShaderGroupStride);
		msg_write(properties.shaderGroupBaseAlignment);
		msg_write(properties.shaderGroupHandleSize);
	}
}

}


}

#endif
