#include "EngineCore.h"

EngineCore::EngineCore()
{

}

void EngineCore::update(const float& frameTime, InputData inputData)
{
	//Update Input
	//Update Player movement(input + physics)
	//Update Enemies
	//Update Collision
	//Update Renderer
	for (auto entity : renderEntities) {
		//Perform Culling
		//Collect indicies of models to be rendered
	}
	//Send compiled list of model indicies to renderer
}

void EngineCore::fixedUpdate(const float& frameTime)
{
	//Update Physics based on component type
}
