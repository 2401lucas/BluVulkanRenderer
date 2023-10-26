#include "SceneUtils.h"

BuildDependancies SceneUtils::getBuildDependencies()
{
    //TODO: LOAD SHADERS FROM SCENES
	BuildDependancies sceneUtils{};
    sceneUtils.shaders.push_back(ShaderInfo(shaderType::VERTEX, "vert.spv"));
    sceneUtils.shaders.push_back(ShaderInfo(shaderType::FRAGMENT, "frag.spv"));
    sceneUtils.materials.push_back(MaterialInfo("textures/viking_room.png"));
    sceneUtils.materials.push_back(MaterialInfo("textures/temp.png"));

    return sceneUtils;
}