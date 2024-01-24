#pragma once
#include "../Input/Input.h"
#include  "../Entity/EntityManager.h"
#include  "../../Render/Renderer/RenderSceneData.h"
#include "../Scene/Scene.h"
#include "../../Render/Textures/TextureManager.h"


class EngineCore {
public:
	EngineCore();
	//TODO: Create EngineTime struct to hold more time data(delta, startup...)
	RenderSceneData update(const float& frameTime, InputData inputData);
	void fixedUpdate(const float& frameTime);
	void loadScene(Scene* scene);

	TextureManager textureManager;

private:
	EntityManager entityManager;
};