#pragma once

#include <vector>
#include "MeshUtils.h"
#include "../Entity/Components/MeshRendererComponent.h"
#include "../../Render/Renderer/RenderModelCreateData.h"

class MeshManager {
public:
	MeshRenderer* registerModel(const char* modelPath);
	std::vector<RenderModelCreateData> getRenderModelCreateData();
	void clear();

private:
	std::vector<MeshData> meshData;
	std::vector<RenderModelCreateData> renderModelCreateData;
};