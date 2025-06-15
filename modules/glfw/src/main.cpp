#include "lib/base/base.h"
#include "KabaExporter.h"
#include <stdio.h>
#include <GLFW/glfw3.h>


extern "C" {


__attribute__ ((visibility ("default")))
void export_symbols(kaba::Exporter* e) {
	//printf("<glfw export>\n");
	e->link("_glfwInit", (void*)&glfwInit);
	e->link("_glfwVulkanSupported", (void*)&glfwVulkanSupported);
	e->link("_glfwCreateWindow", (void*)&glfwCreateWindow);
	e->link("_glfwWindowShouldClose", (void*)&glfwWindowShouldClose);
	e->link("_glfwSetWindowUserPointer", (void*)&glfwSetWindowUserPointer);
	e->link("_glfwGetWindowUserPointer", (void*)&glfwGetWindowUserPointer);
	e->link("_glfwSetKeyCallback", (void*)&glfwSetKeyCallback);
	e->link("_glfwSetCursorPosCallback", (void*)&glfwSetCursorPosCallback);
	e->link("_glfwSetMouseButtonCallback", (void*)&glfwSetMouseButtonCallback);
	e->link("_glfwGetCursorPos", (void*)&glfwGetCursorPos);
	e->link("_glfwWindowHint", (void*)&glfwWindowHint);
	e->link("_glfwPollEvents", (void*)&glfwPollEvents);
	e->link("_glfwTerminate", (void*)&glfwTerminate);
	e->link("_glfwJoystickPresent", (void*)&glfwJoystickPresent);
	e->link("_glfwJoystickIsGamepad", (void*)&glfwJoystickIsGamepad);
	e->link("_glfwGetJoystickName", (void*)&glfwGetJoystickName);
	e->link("_glfwGetGamepadName", (void*)&glfwGetGamepadName);
	e->link("_glfwGetGamepadState", (void*)&glfwGetGamepadState);
}
}


