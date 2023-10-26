#include "Model.h"

Model::Model(const char* modelPath, uint32_t textureIndex)
{
	mesh = new Mesh(modelPath);
	this->textureIndex = textureIndex;
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