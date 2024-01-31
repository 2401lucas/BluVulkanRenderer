#pragma once
#include <map>
#include "../Components/MeshRendererComponent.h"
#include "../Components/MaterialComponent.h"
#include "../Components/TransformComponent.h"
#include "../src/Render/Renderer/RenderModelData.h"
#include "../../../Render/Renderer/RenderSceneData.h"

class RendererSystem {
public:
	static void registerModel(const MeshRenderer* meshData, const MaterialData* materialData, Transform* transformData, RenderSceneData& sceneData, glm::vec4* frustumPlanes, glm::vec4* frustumCorners);
};