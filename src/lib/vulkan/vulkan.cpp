#if HAS_LIB_VULKAN

#include "vulkan.h"

#ifdef HAS_LIB_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "helper.h"
#include "../base/base.h"
#include "../os/msg.h"

//#define NDEBUG



namespace vulkan {

int verbosity = 0;




Instance *init(const Array<string> &op) {
	return Instance::create(op);
}




#ifdef HAS_LIB_GLFW
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
#endif


}

#endif
