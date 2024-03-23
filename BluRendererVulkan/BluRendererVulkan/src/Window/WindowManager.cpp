#include "../Window/WindowManager.h"
#include <stdexcept>


WindowManager::WindowManager(const char* name) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(1920, 1080, name, nullptr, nullptr);
	
	//glfwSetWindowUserPointer(window, nullptr);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	//TODO: 6 CALLBACK FOR WINDOW RESIZE & MINIMIZING
	//glfwSetFramebufferSizeCallback();
}

WindowManager::~WindowManager() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void WindowManager::handleEvents() {
	glfwPollEvents();
}

GLFWwindow* WindowManager::getWindow()
{
	return window;
}