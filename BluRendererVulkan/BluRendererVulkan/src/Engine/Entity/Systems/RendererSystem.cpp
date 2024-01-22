#include "RendererSystem.h"
#include "../../../Render/Math/MathUtils.h"

void RendererSystem::clear()
{
	renderData.clear();
}

void RendererSystem::registerModel(MeshRenderer* meshData, Material* materialData, Transform* transformData)
{
	RenderModelData rmd{};
	rmd.meshRenderData = meshData;
	rmd.materialData = glm::vec3(materialData->textureType, materialData->textureIndex, materialData->materialIndex);
	rmd.modelTransform = MathUtils::TransformToMat4(transformData);
	
	renderData[materialData->pipelineIndex].push_back(rmd);
}

std::map<int, std::vector<RenderModelData>>& RendererSystem::getRenderData()
{
	return renderData;
}
