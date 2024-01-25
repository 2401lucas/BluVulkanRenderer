#include "LightSystem.h"

void LightSystem::registerLight(const Light* light, RenderSceneData& sceneData)
{
	RenderLightData lightData{};
	lightData.lightColor	= light->lightColor;
	lightData.lightPosition = light->lightPosition;
	lightData.lightRotation = light->lightRotation;
	lightData.constant		= light->constant;
	lightData.linear		= light->linear;
	lightData.quad			= light->quad;
	lightData.innerCutoff	= light->innerCutoff;
	lightData.outerCutoff	= light->outerCutoff;
	lightData.lightType		= light->lightType;

	sceneData.lightData.push_back(lightData);
}