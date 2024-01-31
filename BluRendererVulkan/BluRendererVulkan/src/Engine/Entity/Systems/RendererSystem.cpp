#include "RendererSystem.h"
#include "../../../Render/Math/MathUtils.h"

void RendererSystem::registerModel(const MeshRenderer* meshData, const MaterialData* materialData, Transform* transformData, RenderSceneData& sceneData, glm::vec4* frustumPlanes, glm::vec4* frustumCorners)
{
	//if (!MathUtils::isBoxInFrustum(frustumPlanes, frustumCorners, meshData->boundingBox))
	//	return;

	transformData->rotation += glm::vec3(0.01);

	RenderModelData rmd{};
	rmd.meshRenderData = *meshData;
	rmd.materialData = glm::vec3(materialData->textureType, materialData->textureIndex, materialData->materialIndex);
	rmd.modelTransform = MathUtils::TransformToMat4(transformData);
	
	sceneData.modelData[materialData->pipelineIndex].push_back(rmd);
}