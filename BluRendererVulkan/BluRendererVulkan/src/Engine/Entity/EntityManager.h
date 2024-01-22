#pragma once
#include <unordered_map>
#include "EntityArchetypes.h"
#include "EntityChunk/EntityChunkManager.h"
#include "EntityChunk/EntityChunkManager.h"
#include "Components/ComponentManager.h"
#include "Components/EntityComponent.h"
#include "Components/TransformComponent.h"
#include "Components/InputComponent.h"
#include "Components/MaterialComponent.h"
#include "Components/ScriptComponent.h"
#include "Components/MeshRendererComponent.h"
#include "Components/LightComponent.h"
#include "Systems/RendererSystem.h"
#include "../../Render/Renderer/RenderSceneData.h"

class EntityManager {
public:
	void update();

	uint64_t createEntity(uint32_t components, void* componentData);
	void* getEntityData(uint32_t components, uint64_t id);
	RenderSceneData& getRendererData();

private:
	RendererSystem rendererSystem;
	std::unordered_map<uint32_t, EntityChunkManager> registeredEntityArchetypes;
};