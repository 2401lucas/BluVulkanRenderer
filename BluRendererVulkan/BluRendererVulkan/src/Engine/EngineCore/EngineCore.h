#pragma once
#include <unordered_map>
#include "../Input/Input.h"
#include "../Entity/Components/ComponentManager.h"
#include "../Entity/Components/TransformComponent.h"
#include "../Entity/Components/InputComponent.h"
#include "../Entity/Components/MaterialComponent.h"
#include "../Entity/Components/MeshRendererComponent.h"
#include "../Entity/Components/LightComponent.h"
#include "../Entity/EntityArchetypes.h"
#include "../Entity/EntityChunk/EntityChunkManager.h"


class EngineCore {
public:
	EngineCore();
	//TODO: Create EngineTime struct to hold more time data(delta, startup...)
	void update(const float& frameTime, InputData inputData);
	void fixedUpdate(const float& frameTime);
	
	uint64_t createEntity(uint32_t components, void* componentData);
	void* getEntityData(uint32_t components, uint64_t id);

private:
	std::unordered_map<uint32_t, EntityChunkManager> registeredEntityArchetypes;
};