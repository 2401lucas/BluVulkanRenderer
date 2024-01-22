#include "EngineCore.h"
#include "../Entity/Components/EntityComponent.h"

EngineCore::EngineCore()
{

}

void EngineCore::update(const float& frameTime, InputData inputData)
{
	entityManager.update();
}

void EngineCore::fixedUpdate(const float& frameTime)
{
	//Update Physics based on component type
}

RenderSceneData& EngineCore::getRendererData()
{
	entityManager.getRendererData();
}