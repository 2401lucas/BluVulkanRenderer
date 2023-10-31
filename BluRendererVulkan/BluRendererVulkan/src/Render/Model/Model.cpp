#include "Model.h"

Model::Model(const SceneModel& modelInfo, uint32_t textureIndex, uint32_t materialIndex)
	: textureIndex(textureIndex), materialIndex(materialIndex) {
	mesh = new Mesh(modelInfo.modelPath);
	position = modelInfo.position;
	rotation = modelInfo.rotation;
	scale = modelInfo.scale;
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

uint32_t Model::getMaterialIndex()
{
	return materialIndex;
}

glm::vec3& Model::getPosition()
{
	return position;
}

