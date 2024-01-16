#include "Model.h"

Model::Model(const SceneModel& modelInfo, uint32_t textureIndex, uint32_t textureType)
	: textureIndex(textureIndex), textureType(textureType) {
	mesh			= new Mesh(modelInfo.modelPath);
	materialIndex	= modelInfo.materialIndex;
	position		= modelInfo.position;
	rotation		= modelInfo.rotation;
	scale			= modelInfo.scale;
}

void Model::cleanup()
{
	mesh->cleanup();
	delete mesh;
}

Mesh* Model::getMesh()
{
	return mesh;
}

uint32_t Model::getTextureIndex()
{
	return textureIndex;
}

uint32_t Model::getTextureType()
{
	return textureIndex;
}

uint32_t Model::getMaterialIndex()
{
	return materialIndex;
}

glm::vec3& Model::getPosition()
{
	return position;
}

