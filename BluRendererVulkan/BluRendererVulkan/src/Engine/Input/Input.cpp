#include "Input.h"


Input::Input()
{
}

void Input::clear()
{
	framebufferResized = false;
	pressedKeys.clear();
	pressedButtons.clear();
	curX = 0;
	curY = 0;
}

void Input::onKeyPressed(int key, int scancode, int action, int mods)
{
	pressedKeys.insert(key);
}

void Input::onCursorPos(double x, double y)
{
	curX = x;
	curY = y;
}

void Input::onButtonPressed(int button, int action, int mods)
{
	pressedButtons.insert(button);
}

void Input::onFramebufferResized()
{
	framebufferResized = true;
}

bool Input::isFramebufferResized()
{
	return framebufferResized;
}

InputData Input::getInput()
{
	return InputData(pressedKeys, pressedButtons, curX, curY);
}