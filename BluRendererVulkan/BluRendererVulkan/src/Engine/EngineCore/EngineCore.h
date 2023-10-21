#pragma once
#include "../Input/Input.h"

class EngineCore {
public:
	EngineCore();
	void update(const float& frameTime, InputData inputData);
	void updatePhysics(const float& frameTime);

private:
	
	
};