#pragma once
#include <map>
#include "RenderModelData.h"
#include "RenderLightData.h"
#include "RenderCameraData.h"

struct RenderSceneData {
public:
	std::map<int, std::vector<RenderModelData>> modelData;
	std::vector<RenderLightData> lightData;
	RenderCameraData cameraData;
};