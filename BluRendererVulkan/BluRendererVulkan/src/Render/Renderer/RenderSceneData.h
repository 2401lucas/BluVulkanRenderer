#pragma once
#include <map>
#include <vector>
#include "RenderMeshCreateData.h"
#include "RenderModelData.h"
#include "RenderLightData.h"
#include "RenderCameraData.h"

struct RenderSceneData {
public:
	std::map<int, std::vector<RenderModelData>> modelData;
	std::vector<RenderLightData> lightData;
	RenderCameraData cameraData;
	std::vector<RenderMeshCreateData> modelCreateData;
};