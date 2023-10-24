#include "Model.h"
#include "../Image/ImageUtils.h"

Model::Model(Device* deviceInfo, CommandPool* commandPool, ModelCreateInfo createInfo)
{
	mesh = new Mesh(createInfo.modelPath);
	texture = ImageUtils::createImageFromPath(deviceInfo, commandPool, createInfo.texturePath);
}

void Model::cleanup(Device* deviceInfo)
{
	texture->cleanup(deviceInfo);
	delete texture;
	mesh->cleanup();
	delete mesh;
}

Mesh* Model::getMesh()
{
	return mesh;
}

Image* Model::getTexture()
{
	return texture;
}