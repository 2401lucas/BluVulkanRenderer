#include "Input.h"


Input::Input()
{
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

InputData Input::getInput()
{
	return InputData(pressedKeys, pressedButtons, curX, curY);
}