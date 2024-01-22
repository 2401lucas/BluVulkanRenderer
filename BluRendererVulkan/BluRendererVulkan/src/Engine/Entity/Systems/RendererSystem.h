#pragma once
#include <map>
#include "../Components/MeshRendererComponent.h"
#include "../Components/MaterialComponent.h"
#include "../Components/TransformComponent.h"
#include "../src/Render/Renderer/RenderModelData.h"

class RendererSystem {
public:
	void clear();
	void registerModel(MeshRenderer* meshData, Material* materialData, Transform* transformData);
	std::map<int, std::vector<RenderModelData>>& getRenderData();

private:
	std::map<int, std::vector<RenderModelData>> renderData;
};