#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../src/Engine/Input/Input.h"

class WindowManager {
public:
	WindowManager(const char*);
	~WindowManager();

	void handleEvents();
	InputData getInput();
	GLFWwindow* getWindow();
	bool isFramebufferResized();

	enum class CursorMode {
		Normal,
		Hidden,
		Disabled,
	};

private:
	
	GLFWwindow* window;
	Input input;
};