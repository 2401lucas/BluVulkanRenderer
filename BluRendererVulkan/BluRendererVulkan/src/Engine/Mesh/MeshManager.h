#pragma once

#include <vector>
#include "MeshUtils.h"
#include "../Entity/Components/MeshRendererComponent.h"
#include "../../Render/Renderer/RenderModelCreateData.h"

class MeshManager {
public:
	RenderModelCreateData registerModel(const char* modelPath, MeshRenderer* mr);

private:
	std::vector<MeshData> meshData;
};