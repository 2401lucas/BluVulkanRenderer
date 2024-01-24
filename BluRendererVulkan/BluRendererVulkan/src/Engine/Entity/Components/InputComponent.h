#pragma once
#include "BaseComponent.h"

struct Input : BaseComponent {
public:
	float mouseX;
	float mouseY;
	float horizontal;
	float vertical;
};