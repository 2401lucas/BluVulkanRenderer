#include "../Window/WindowManager.h"
#include <stdexcept>


WindowManager::WindowManager(const char* name) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(640, 480, name, nullptr, nullptr);
	
	//TODO: 6 CALLBACK FOR WINDOW RESIZE & MINIMIZING
	//glfwSetWindowUserPointer()
	//glfwSetFramebufferSizeCallback();
}

WindowManager::~WindowManager() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void WindowManager::HandleEvents() {
	glfwPollEvents();
}

GLFWwindow* WindowManager::GetWindow()
{
	return window;
}

void errorCallback(int error, const char* description)
{
	throw std::runtime_error(description);
}