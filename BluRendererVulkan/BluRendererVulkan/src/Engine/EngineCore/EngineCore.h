#pragma once
#include "../Input/Input.h"
#include  "../../Render/Renderer/RenderSceneData.h"
#include "../Scene/Scene.h"

#include  "../Entity/EntityManager.h"
#include "../../Render/Textures/TextureManager.h"
#include "../Mesh/MeshManager.h"
#include "../src/Render/Renderer/RenderManager.h"


class EngineCore {
public:
	EngineCore(GLFWwindow* window, const VkApplicationInfo& appInfo, DeviceSettings deviceSettings, Scene* scene);
	//TODO: Create EngineTime struct to hold more time data(delta, startup...)
	RenderSceneData update(const float& frameTime, InputData inputData, bool frameBufferResized);
	void fixedUpdate(const float& frameTime);
	void loadScene(Scene* scene);


private:
	RenderManager* renderManager;
	MeshManager* meshManager;
	TextureManager* textureManager;
	EntityManager* entityManager;
};