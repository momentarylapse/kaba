#include "lib/base/base.h"
//#include "lib/kaba/kaba.h"
//#include "lib/kaba/lib/lib.h"
#include <stdio.h>
#include <GLFW/glfw3.h>

class KabaExporter {
public:
	virtual ~KabaExporter() = default;
	virtual void declare_class_size(const string& name, int size) = 0;
	virtual void declare_enum(const string& name, int value) = 0;
	virtual void declare_class_element(const string& name, int offset) = 0;
	virtual void link(const string& name, void* p) = 0;
	virtual void link_virtual(const string& name, void* p, void* instance) = 0;
};


extern "C" {

int _glfwVulkanSupported() {
	return glfwVulkanSupported();
}
/*func extern _glfwCreateWindow(width: int, height: int, title: u8*, monitor: void*, share: void*) -> void*
func extern _glfwWindowShouldClose(window: void*) -> int
func extern _glfwSetWindowUserPointer(window: void*, p: void*)
func extern _glfwGetWindowUserPointer(window: void*) -> void&
func extern _glfwSetKeyCallback(window: void*, f: void*) -> void*
func extern _glfwSetCursorPosCallback(window: void*, f: void*) -> void*
func extern _glfwSetMouseButtonCallback(window: void*, f: void*) -> void*
func extern _glfwGetCursorPos(window: void*, x: f64*, y: f64*)
func extern _glfwWindowHint(hint: int, value: int)
func extern _glfwPollEvents()
func extern _glfwTerminate()
func extern _glfwJoystickPresent(j: int) -> int
func extern _glfwJoystickIsGamepad(j: int) -> int
func extern _glfwGetJoystickName(j: int) -> u8[0]*
func extern _glfwGetGamepadName(j: int) -> u8[0]*
func extern _glfwGetGamepadState(j: int, p: void*) -> int*/

void export_symbols(KabaExporter* e) {
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


