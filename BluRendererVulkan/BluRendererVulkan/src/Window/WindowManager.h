#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class WindowManager {
public:
	WindowManager(const char*);
	~WindowManager();

	void HandleEvents();

	int GetWidth();
	int GetHeight();

	void SetFullscreen(bool fullscreen = true);

	GLFWwindow* GetWindow();

	enum class CursorMode {
		Normal,
		Hidden,
		Disabled,
	};

private:
	void errorCallback(int, const char*);
	GLFWwindow* window;
};