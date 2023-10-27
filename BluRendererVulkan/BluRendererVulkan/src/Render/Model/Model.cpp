#include "Model.h"

Model::Model(const SceneModel& modelInfo, uint32_t textureIndex)
{
	mesh = new Mesh(modelInfo.modelPath);
	this->textureIndex = textureIndex;
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

glm::vec3& Model::getPosition()
{
	return position;
}

