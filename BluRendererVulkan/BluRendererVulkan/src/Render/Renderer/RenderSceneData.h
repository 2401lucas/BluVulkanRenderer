#pragma once
#include <map>
#include "RenderModelData.h"
#include "RenderLightData.h"

class RenderSceneData {
public:
	std::map<int, std::vector<RenderModelData>> modelData;
	RenderLightData lightData;
};