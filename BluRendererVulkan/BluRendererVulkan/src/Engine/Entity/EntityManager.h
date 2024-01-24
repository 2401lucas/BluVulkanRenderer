#pragma once
#include <map>
#include "EntityChunk/EntityChunkManager.h"
#include "Systems/RendererSystem.h"
#include "../../Render/Renderer/RenderSceneData.h"

class EntityManager {
public:
	RenderSceneData update();

	uint64_t createEntity(uint32_t components, std::vector<BaseComponent*> componentData);
	std::vector<BaseComponent*> getEntityData(uint32_t components, uint64_t id);

private:
	std::map<uint32_t, EntityChunkManager> registeredEntityArchetypes;
};