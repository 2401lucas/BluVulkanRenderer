#pragma once
#include "../Input/Input.h"
#include  "../../Render/Renderer/RenderSceneData.h"
#include  "../Entity/EntityManager.h"
#include "../../Render/Textures/TextureManager.h"
#include "../Mesh/MeshManager.h"
#include "../src/Render/Renderer/RenderManager.h"
#include "../Scene/SceneManager.h"


class EngineCore {
public:
	EngineCore(GLFWwindow* window, const VkApplicationInfo& appInfo, DeviceSettings deviceSettings);
	//TODO: Create EngineTime struct to hold more time data(delta, startup...)
	RenderSceneData update(const float& frameTime, InputData inputData, bool frameBufferResized);
	void fixedUpdate(const float& frameTime);
	void loadScene(const char* scenePath);


private:
	SceneManager* sceneManager;
	RenderManager* renderManager;
	MeshManager* meshManager;
	EntityManager* entityManager;

	Scene* activeScene;
};