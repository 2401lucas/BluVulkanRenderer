#include "MeshManager.h"

RenderModelCreateData MeshManager::registerModel(const char* modelPath, MeshRenderer* mr)
{
	MeshData md;
	md.meshPath = modelPath;
	MeshUtils::getMeshDataFromPath(&md);
	meshData.push_back(md);

	mr->boundingBox = md.boundingBox;
	mr->indexCount = md.indices.size();
	
	RenderModelCreateData modelCreateData{};
	modelCreateData.indices = md.indices;
	modelCreateData.vertices = md.vertices;

	return modelCreateData;
}