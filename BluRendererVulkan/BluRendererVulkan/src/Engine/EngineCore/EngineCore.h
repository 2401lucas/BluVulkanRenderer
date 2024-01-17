#pragma once
#include "../Input/Input.h"
#include "../Entity/Components/ComponentManager.h"
#include "../Entity/Components/TransformComponent.h"
#include "../Entity/Components/InputComponent.h"
#include "../Entity/Components/MaterialComponent.h"
#include "../Entity/Components/MeshRendererComponent.h"
#include "../Entity/Components/LightComponent.h"


class EngineCore {
public:
	EngineCore();
	//TODO: Create EngineTime struct to hold more time data(delta, startup...)
	void update(const float& frameTime, InputData inputData);
	void fixedUpdate(const float& frameTime);

private:
	//Component Managers
	ComponentManager<Transform> transformManager;
	ComponentManager<Input> inputManager;
	ComponentManager<Material> materialManager;
	ComponentManager<MeshRenderer> meshRendererManager;
	ComponentManager<Light> lightManager;

	//ArchetypeLists
};