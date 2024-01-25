#include "MeshManager.h"

MeshRenderer* MeshManager::registerModel(const char* modelPath)
{
	MeshData md;
	md.meshPath = modelPath;
	MeshUtils::getMeshDataFromPath(&md);
	meshData.push_back(md);

	MeshRenderer* mr = new MeshRenderer();
	mr->boundingBox = md.boundingBox;
	mr->indexCount = md.indices.size();
	
	RenderModelCreateData modelCreateData{};
	modelCreateData.indices = md.indices;
	modelCreateData.vertices = md.vertices;
	modelCreateData.meshRenderer = mr;
	renderModelCreateData.push_back(modelCreateData);

	return mr;
}

std::vector<RenderModelCreateData> MeshManager::getRenderModelCreateData()
{
	return renderModelCreateData;
}

void MeshManager::clear()
{
	renderModelCreateData.clear();
}
