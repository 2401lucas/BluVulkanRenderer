#pragma once
#include "../Input/Input.h"
#include  "../Entity/EntityManager.h"
#include  "../../Render/Renderer/RenderSceneData.h"


class EngineCore {
public:
	EngineCore();
	//TODO: Create EngineTime struct to hold more time data(delta, startup...)
	void update(const float& frameTime, InputData inputData);
	void fixedUpdate(const float& frameTime);
	RenderSceneData& getRendererData();

private:
	EntityManager entityManager;
};