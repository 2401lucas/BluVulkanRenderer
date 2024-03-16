#include "Scene.h"

Scene::Scene(const char* scenePath) {
  info = new SceneInfo();
  // TODO: LOAD FROM FILE

  // Scene Info
  info->ambientColor = glm::vec4(0.6, 0.5, 0.1, 1);
  info->fogColor = glm::vec4(0.048f, 0.048f, 0.048f, 1.0f);  // Light Gray
  info->fogDistances = glm::vec4(1.0f, 10.0f, 0.0f, 0.0f);

  // Lighting----------------------
  info->lights.push_back(SceneLight(glm::vec3(2.0f, 2.0f, 2.0f),
                                    glm::vec4(1.0, 1.0, 1.0, 100), 1, 0.09f,
                                    0.032f));

  // Models----------------------------
  info->dynamicModels.push_back(
      SceneModel("models/cube.obj", "materials/cube.png", 0, 1,
                 glm::vec3(0.0f, -0.0f, -0.0f), glm::vec3(45.0f, 45.0f, 0.0f),
                 glm::vec3(1.0f, 1.0f, 1.0f)));

  // Camera
  info->cameras.push_back(
      SceneCamera(glm::vec3(20.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  1.570796326 /*90 deg in rad*/, 16 / 9, 0.1f, 100.0f));

  sceneDependancies = new SceneDependancies();
  // Shaders
  sceneDependancies->shaderPairs.push_back(std::pair<ShaderInfo, ShaderInfo>(
      ShaderInfo(shaderType::VERTEX, "vert.spv"),
      ShaderInfo(shaderType::FRAGMENT, "frag.spv")));

    //sceneDependancies->basicMaterials.push_back("materials/default.mat");
    sceneDependancies->basicMaterials.push_back("materials/cube.png");
}

void Scene::cleanup() {
  delete info;
  delete sceneDependancies;
}

SceneInfo* Scene::getSceneInfo() { return info; }

SceneDependancies* Scene::getSceneDependancies() { return sceneDependancies; }