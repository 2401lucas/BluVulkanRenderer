#include "EngineCore.h"

EngineCore::EngineCore()
{

}

void EngineCore::update(const float& frameTime, InputData inputData)
{
	//Loop thru archetype lists & check bits
	for (auto& archetype : registeredEntityArchetypes) {
		for(auto& entity : archetype.second.getData()) {
			if ((archetype.first & ComponentTypes::InputComponent) == ComponentTypes::InputComponent)
			{

			}
			else if ((archetype.first & ComponentTypes::ScriptComponent) == ComponentTypes::ScriptComponent)
			{

			}
			else if ((archetype.first & ComponentTypes::TransformComponent) == ComponentTypes::TransformComponent)
			{

			}
			else if ((archetype.first & ComponentTypes::LightComponent) == ComponentTypes::LightComponent)
			{

			}
			else if ((archetype.first & ComponentTypes::MaterialComponent) == ComponentTypes::MaterialComponent)
			{

			}
			else if ((archetype.first & ComponentTypes::MeshRendererComponent) == ComponentTypes::MeshRendererComponent)
			{

			}
		}
	}

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

uint64_t EngineCore::createEntity(uint32_t components, void* data)
{
	if (registeredEntityArchetypes.count(components) == 0) {
		registeredEntityArchetypes[components] = EntityChunkManager();
	}

	return registeredEntityArchetypes[components].addEntityData(data);
}

void* EngineCore::getEntityData(uint32_t components, uint64_t id)
{
	return registeredEntityArchetypes[components].getData(id);
}