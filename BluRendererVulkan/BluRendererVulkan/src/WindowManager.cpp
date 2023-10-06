#include "../include/WindowManager.h"


Core::System::WindowManager::WindowManager() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(800, 600, "BluRendererVulkan", nullptr, nullptr);
	
	//TODO : CALLBACK FOR WINDOW RESIZE & MINIMIZING
	//glfwSetWindowUserPointer()
	//glfwSetFramebufferSizeCallback();
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