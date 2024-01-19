#pragma once
#include <vector>
#include "Entity.h"

enum ComponentTypes{
	InputComponent = 1,
	ScriptComponent = 2,
	TransformComponent = 4,
	LightComponent = 8,
	MaterialComponent = 16,
	MeshRendererComponent = 32,
};
//Component Chunk
//List Generic Components{Type, Data}

class EntityArchetypes {
public:
	bool checkComponents(uint32_t componentTypes);

private:
	uint32_t numOfEntities;
	uint32_t unusedEntities;
	uint32_t componentsTypes;
	uint32_t chunks;
};