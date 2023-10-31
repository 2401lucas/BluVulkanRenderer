#include "SceneUtils.h"

BuildDependancies SceneUtils::getBuildDependencies()
{
    //TODO: LOAD SHADERS FROM SCENES
	BuildDependancies sceneUtils{};
    sceneUtils.shaders.push_back(ShaderInfo(shaderType::VERTEX, "vert.spv"));
    sceneUtils.shaders.push_back(ShaderInfo(shaderType::FRAGMENT, "frag.spv"));
    sceneUtils.textures.push_back(TextureInfo("textures/Container/container", ".png", true));
    //Emerald
    sceneUtils.materials.push_back(MaterialInfo(glm::vec3(0.0215f, 0.1745f, 0.0215f), glm::vec3(0.07568f, 0.61424f, 0.07568f), glm::vec3(0.633f, 0.727811f, 0.633f), 0.6));
    //Chrome
    sceneUtils.materials.push_back(MaterialInfo(glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.774597f, 0.774597f, 0.774597f), 0.6));
    //Black Plastic
    sceneUtils.materials.push_back(MaterialInfo(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.01f, 0.01f, 0.01f), glm::vec3(0.50f, 0.50f, 0.50f), 0.25));

    return sceneUtils;
}