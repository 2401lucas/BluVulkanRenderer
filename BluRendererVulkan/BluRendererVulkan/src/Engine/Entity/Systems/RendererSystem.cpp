#include "RendererSystem.h"
#include "../../../Render/Math/MathUtils.h"

void RendererSystem::registerModel(const MeshRenderer* meshData, const MaterialData* materialData, const Transform* transformData, RenderSceneData& sceneData)
{
	RenderModelData rmd{};
	rmd.meshRenderData = *meshData;
	rmd.materialData = glm::vec3(materialData->textureType, materialData->textureIndex, materialData->materialIndex);
	rmd.modelTransform = MathUtils::TransformToMat4(transformData);
	
	sceneData.modelData[materialData->pipelineIndex].push_back(rmd);
}