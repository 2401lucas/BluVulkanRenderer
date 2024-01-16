#pragma once
#include <vector>
#include <memory>
#include "../Mesh/Mesh.h"
#include "../src/Engine/Scene/SceneUtils.h"
#include "Components/BaseComponent.h"

class Model {
public:
	Model(const SceneModel& modelPath);
    void cleanup();

	BaseComponent* getComponent(uint32_t componentID);

	uint32_t componentID;
private:
	//TODO: Ensure memory is contiguous 
	std::vector<BaseComponent*> components;
};