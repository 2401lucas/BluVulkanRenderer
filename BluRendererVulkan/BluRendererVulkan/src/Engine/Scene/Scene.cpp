#include "Scene.h"

Scene::Scene(const char* scenePath)
{
	info = new SceneInfo();
	//TODO: LOAD FROM FILE
	
	//Scene Wide
	info->ambientColor = glm::vec4(0.6, 0.5, 0.1, 0.1);
	info->fogColor = glm::vec4(0.048f, 0.048f, 0.048f, 1.0f); // Light Gray
	info->fogDistances = glm::vec4(1.0f, 10.0f, 0.0f, 0.0f);
	
	//Lighting----------------------
	info->lights.push_back(SceneLight(glm::vec3(-5.0f, -5.0f, 3.0f), glm::vec4(1.0, 1.0, 1.0, 5), 1, 0.09f, 0.032f));
	
	//Models----------------------------
	info->dynamicModels.push_back(SceneModel("models/cube.obj", "textures/blue.png", 0, 1, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
	
	//Camera
	info->cameras.push_back(SceneCamera(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), 0.785398/*45 deg in rad*/, 16/9, 0.1f, 10.0f));
}

void Scene::cleanup()
{
	delete info;
}

SceneInfo* Scene::getSceneInfo()
{
	return info;
}

SceneDependancies Scene::getSceneDependancies()
{
	//TODO: ASSIGN WHEN LOADING SCENES
	SceneDependancies sceneDepen{};
	//Shaders
	sceneDepen.shaders.push_back(ShaderInfo(shaderType::VERTEX, "vert.spv"));
	sceneDepen.shaders.push_back(ShaderInfo(shaderType::FRAGMENT, "frag.spv"));
	
	//Textures
	sceneDepen.textures.push_back(TextureInfo("textures/blue", ".png", TextureType::Phong));
	
	//Materials

	//Emerald
	sceneDepen.materials.push_back(MaterialInfo(glm::vec3(0.0215f, 0.1745f, 0.0215f), glm::vec3(0.07568f, 0.61424f, 0.07568f), glm::vec3(0.633f, 0.727811f, 0.633f), 0.6));
	//Chrome
	sceneDepen.materials.push_back(MaterialInfo(glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.774597f, 0.774597f, 0.774597f), 0.6));
	//Black Plastic
	sceneDepen.materials.push_back(MaterialInfo(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.01f, 0.01f, 0.01f), glm::vec3(0.50f, 0.50f, 0.50f), 0.25));

	return sceneDepen;
}