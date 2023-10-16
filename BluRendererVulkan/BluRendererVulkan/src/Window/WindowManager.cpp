#include "../Window/WindowManager.h"
#include <stdexcept>


Core::System::WindowManager::WindowManager(VkInstance instance, const char* name) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(640, 480, name, nullptr, nullptr);
	
	//TODO : CALLBACK FOR WINDOW RESIZE & MINIMIZING
	//glfwSetWindowUserPointer()
	//glfwSetFramebufferSizeCallback();
	
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");
}

Core::System::WindowManager::~WindowManager() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Core::System::WindowManager::HandleEvents() {
	glfwPollEvents();
}

GLFWwindow* Core::System::WindowManager::GetWindow()
{
	return window;
}

VkSurfaceKHR Core::System::WindowManager::GetSurface()
{
	return surface;
}

void Core::System::WindowManager::errorCallback(int error, const char* description)
{
	throw std::runtime_error(description);
}