#include "EntityManager.h"

#include "EntityArchetypes.h"
#include "Components/ComponentManager.h"
#include "Components/EntityComponent.h"
#include "Components/TransformComponent.h"
#include "Components/InputComponent.h"
#include "Components/MaterialComponent.h"
#include "Components/ScriptComponent.h"
#include "Components/MeshRendererComponent.h"
#include "Components/LightComponent.h"
#include "Components/CameraComponent.h"
#include "Systems/CameraSystem.h"
#include "Systems/LightSystem.h"
#include "../../Render/Math/MathUtils.h"


RenderSceneData EntityManager::update() {
	RenderSceneData rendererData{};
	
	glm::vec4 frustumPlanes[6];
	glm::vec4 frustumCorners[8];

	for (auto& archetype : registeredEntityArchetypes) {
		for (uint32_t i = 0; i < archetype.second.getSize(); i++) {
			int index = 0;
			std::vector<BaseComponent*> components = archetype.second.getData(i);
			EntityData* entityData = static_cast<EntityData*>(components[index]);
			if (!entityData->isActive)
				continue;
			index++;

			Input* inputData;
			if ((archetype.first & ComponentTypes::InputComponent) == ComponentTypes::InputComponent)
			{
				inputData = static_cast<Input*>(components.at(index));
				index++;

				//Process input
			}
			Script* scriptData;
			if ((archetype.first & ComponentTypes::ScriptComponent) == ComponentTypes::ScriptComponent)
			{
				scriptData = static_cast<Script*>(components.at(index));
				index++;
			}
			Transform* transformData;
			if ((archetype.first & ComponentTypes::TransformComponent) == ComponentTypes::TransformComponent)
			{
				transformData = static_cast<Transform*>(components.at(index));
				index++;
			}
			Camera* cameraData;
			if ((archetype.first & ComponentTypes::CameraComponent) == ComponentTypes::CameraComponent)
			{
				cameraData = static_cast<Camera*>(components.at(index));
				index++;

				CameraSystem::updateCamera(cameraData, transformData);

				MathUtils::getFrustumPlanes(cameraData->proj * cameraData->view, frustumPlanes);
				MathUtils::getFrustumCorners(cameraData->proj * cameraData->view, frustumCorners);

				rendererData.cameraData.position = transformData->position;
				rendererData.cameraData.projMat = cameraData->proj;
				rendererData.cameraData.viewMat = cameraData->view;
			}
			//Update Enemies
			//Update Collision
			Light* lightData;
			if ((archetype.first & ComponentTypes::LightComponent) == ComponentTypes::LightComponent)
			{
				lightData = static_cast<Light*>(components.at(index));
				index++;

				LightSystem::registerLight(lightData, rendererData);
			}
			MaterialData* matData;
			if ((archetype.first & ComponentTypes::MaterialComponent) == ComponentTypes::MaterialComponent)
			{
				matData = static_cast<MaterialData*>(components.at(index));
				index++;
			}
			MeshRenderer* meshRendererData;
			if ((archetype.first & ComponentTypes::MeshRendererComponent) == ComponentTypes::MeshRendererComponent)
			{
				meshRendererData = static_cast<MeshRenderer*>(components.at(index));
				index++;

				//Perform Culling
				//Collect indicies of models to be rendered
				RendererSystem::registerModel(meshRendererData, matData, transformData, rendererData, frustumPlanes, frustumCorners);
			}
		}
	}

	return rendererData;
}

uint64_t EntityManager::createEntity(uint32_t components, std::vector<BaseComponent*> data)
{
	if (registeredEntityArchetypes.count(components) == 0) {
		registeredEntityArchetypes[components] = EntityChunkManager();
	}

	//Create Components
	//Save components
	//Whats missing:
	//Load data onto GPU

	return registeredEntityArchetypes[components].addEntityData(data);
}

std::vector<BaseComponent*> EntityManager::getEntityData(uint32_t components, uint64_t id)
{
	return registeredEntityArchetypes[components].getData(id);
}