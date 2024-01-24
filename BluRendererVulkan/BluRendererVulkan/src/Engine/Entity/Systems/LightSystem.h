#pragma once
#include "../Components/LightComponent.h"
#include "../../../Render/Renderer/RenderSceneData.h"

class LightSystem {
public:
	static void registerLight(const Light* light, RenderSceneData& sceneData);
};