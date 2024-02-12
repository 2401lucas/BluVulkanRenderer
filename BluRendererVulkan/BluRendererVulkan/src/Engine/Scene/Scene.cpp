#include "Scene.h"

Scene::Scene(const char* scenePath) {
  info = new SceneInfo();
  // TODO: LOAD FROM FILE

  // Scene Wide
  info->ambientColor = glm::vec4(0.6, 0.5, 0.1, 1);
  info->fogColor = glm::vec4(0.048f, 0.048f, 0.048f, 1.0f);  // Light Gray
  info->fogDistances = glm::vec4(1.0f, 10.0f, 0.0f, 0.0f);

  // Lighting----------------------
  info->lights.push_back(SceneLight(glm::vec3(2.0f, 2.0f, 2.0f),
                                    glm::vec4(1.0, 1.0, 1.0, 100), 1, 0.09f,
                                    0.032f));

  // Models----------------------------
  // info->dynamicModels.push_back(SceneModel("models/cube.obj",
  // TextureInfo("textures/blue", ".png", TextureType::Phong), 0, 1,
  // glm::vec3(-2.0f, -2.0f, -2.0f), glm::vec3(45.0f, 45.0f, 0.0f),
  // glm::vec3(1.0f, 1.0f, 1.0f)));
  info->dynamicModels.push_back(
      SceneModel("models/cube.obj",
                 TextureInfo("textures/blue", ".png", TextureType::Phong), 0, 1,
                 glm::vec3(0.0f, -0.0f, -0.0f), glm::vec3(45.0f, 45.0f, 0.0f),
                 glm::vec3(1.0f, 1.0f, 1.0f)));
  // info->dynamicModels.push_back(SceneModel("models/SpyCar.obj",
  // TextureInfo("textures/SpyCar", ".png", TextureType::Phong), 0, 1,
  // glm::vec3(.0f, .0f, .0f), glm::vec3(90.0f, 180.0f, 0.0f), glm::vec3(.1f,
  // .1f, .1f)));

  // Camera
  info->cameras.push_back(
      SceneCamera(glm::vec3(20.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  1.570796326 /*90 deg in rad*/, 16 / 9, 0.1f, 100.0f));

  sceneDependancies = new SceneDependancies();
  // Shaders
  sceneDependancies->shaderPairs.push_back(std::pair<ShaderInfo, ShaderInfo>(
      ShaderInfo(shaderType::VERTEX, "vert.spv"),
      ShaderInfo(shaderType::FRAGMENT, "frag.spv")));

  // Textures
  sceneDependancies->textures.push_back(
      TextureInfo("textures/blue", ".png", TextureType::Phong));
  sceneDependancies->textures.push_back(
      TextureInfo("textures/SpyCar", ".png", TextureType::Phong));
  // Materials

  // Emerald
  sceneDependancies->materials.push_back(
      MaterialInfo(glm::vec3(0.0215f, 0.1745f, 0.0215f),
                   glm::vec3(0.07568f, 0.61424f, 0.07568f),
                   glm::vec3(0.633f, 0.727811f, 0.633f), 0.6));
  // Chrome
  sceneDependancies->materials.push_back(
      MaterialInfo(glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.4f, 0.4f, 0.4f),
                   glm::vec3(0.774597f, 0.774597f, 0.774597f), 0.6));
  // Black Plastic
  sceneDependancies->materials.push_back(
      MaterialInfo(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.01f, 0.01f, 0.01f),
                   glm::vec3(0.50f, 0.50f, 0.50f), 0.25));
}

void Scene::cleanup() {
  delete info;
  delete sceneDependancies;
}

SceneInfo* Scene::getSceneInfo() { return info; }

SceneDependancies* Scene::getSceneDependancies() { return sceneDependancies; }