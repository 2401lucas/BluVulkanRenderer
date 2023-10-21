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

InputData WindowManager::getInput()
{
	return input.getInput();
}

GLFWwindow* WindowManager::getWindow()
{
	return window;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		// Key is pressed
		if (key == GLFW_KEY_ESCAPE) {
			// Handle the ESC key
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	// Handle mouse cursor movement
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	// Handle mouse button clicks
}