#include "EntityManager.h"

//TODO:
// 5 Perform culling on models
void EntityManager::update() {
	rendererSystem.clear();

	for (auto& archetype : registeredEntityArchetypes) {
		for (uint32_t i = 0; i < archetype.second.getSize(); i++) {
			char* data = (char*)archetype.second.getData(i);
			if (!static_cast<EntityData>(data).isActive)
				return;
			int offset = sizeof(EntityData);
			Input* inputData;
			if ((archetype.first & ComponentTypes::InputComponent) == ComponentTypes::InputComponent)
			{
				inputData = (Input*)(data + offset);
				offset += sizeof(Input);

				//Process input
			}
			Script* scriptData;
			if ((archetype.first & ComponentTypes::ScriptComponent) == ComponentTypes::ScriptComponent)
			{
				scriptData = (Script*)(data + offset);
				offset += sizeof(Script*);
			}
			Transform* transformData;
			if ((archetype.first & ComponentTypes::TransformComponent) == ComponentTypes::TransformComponent)
			{
				transformData = (Transform*)(data + offset);
				offset += sizeof(Transform*);
			}
			//Update Enemies
			//Update Collision
			Light* lightData;
			if ((archetype.first & ComponentTypes::LightComponent) == ComponentTypes::LightComponent)
			{
				lightData = (Light*)(data + offset);
				offset += sizeof(Light*);
			}
			Material* matData;
			if ((archetype.first & ComponentTypes::MaterialComponent) == ComponentTypes::MaterialComponent)
			{
				matData = (Material*)(data + offset);
				offset += sizeof(Material*);
			}
			MeshRenderer* meshRendererData;
			if ((archetype.first & ComponentTypes::MeshRendererComponent) == ComponentTypes::MeshRendererComponent && matData != nullptr && transformData != nullptr)
			{
				meshRendererData = (MeshRenderer*)(data + offset);
				offset += sizeof(MeshRenderer*);

				//Perform Culling
				//Collect indicies of models to be rendered
				rendererSystem.registerModel(meshRendererData, matData, transformData);
			}
		}
	}
}

uint64_t EntityManager::createEntity(uint32_t components, void* data)
{
	if (registeredEntityArchetypes.count(components) == 0) {
		registeredEntityArchetypes[components] = EntityChunkManager();
	}

	return registeredEntityArchetypes[components].addEntityData(data);
}

void* EntityManager::getEntityData(uint32_t components, uint64_t id)
{
	return registeredEntityArchetypes[components].getData(id);
}

//TODO: Add lighting Data
RenderSceneData& EntityManager::getRendererData()
{
	RenderSceneData sceneData {};
	sceneData.modelData = rendererSystem.getRenderData();
}
