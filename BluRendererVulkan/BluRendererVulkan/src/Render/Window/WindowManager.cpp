#include "../Window/WindowManager.h"
#include <stdexcept>


WindowManager::WindowManager(const char* name) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(640, 480, name, nullptr, nullptr);
	
	glfwSetWindowUserPointer(window, &input);

	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
		input->onKeyPressed(key, scancode, action, mods);
	});

	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
		Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
		input->onCursorPos(x, y);
	});

	glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
		Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
		input->onButtonPressed(button, action, mods);
	});
	
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
		input->onFramebufferResized();
		});

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
	input.clear();
	glfwPollEvents();
}

InputData WindowManager::getInput()
{
	return input.getInput();
}

GLFWwindow* WindowManager::getWindow()
{
	return window;
}

bool WindowManager::isFramebufferResized()
{
	return input.isFramebufferResized();
}

