#pragma once

#include <set>
#include <GLFW/glfw3.h>

struct InputData {
	std::set<int> pressedKeys;
	std::set<int> pressedButtons;
	double curX;
	double curY;
	
	InputData(std::set<int> pressedKeys, std::set<int> pressedButtons, double curX, double curY) 
		: pressedKeys(pressedKeys), pressedButtons(pressedButtons), curX(curX), curY(curY) {

	}

	bool isKeyPressed(int key) {
		return pressedKeys.find(key) != pressedKeys.end();
	}

	bool isButtonPressed(int button) {
		return pressedButtons.find(button) != pressedButtons.end();
	}

	double getMouseX() {
		return curX;
	}

	double getMouseY() {
		return curY;
	}
};

class Input {
public:
	Input();

	void clear();
	void onKeyPressed(int key, int scancode, int action, int mods);
	void onCursorPos(double x, double y);
	void onButtonPressed(int button, int action, int mods);
	void onFramebufferResized();
	bool isFramebufferResized();

	InputData getInput();
private:
	bool framebufferResized;
	std::set<int> pressedKeys;
	std::set<int> pressedButtons;
	double curX;
	double curY;
};