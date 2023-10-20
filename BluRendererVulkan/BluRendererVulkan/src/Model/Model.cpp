#include "Model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Model::Model(Device* deviceInfo, CommandPool* commandPool, const char* modelPath, const char* texturePath)
	: modelPath(modelPath), texturePath(texturePath)
{
	loadModel();
	loadTexture(deviceInfo, commandPool);
}

void Model::cleanup(Device* deviceInfo)
{
	texture->cleanup(deviceInfo);
	delete texture;
}

std::vector<Vertex> Model::getVertices()
{
	return vertices;
}

std::vector<uint32_t> Model::getIndices()
{
	return indices;
}

Image* Model::getTexture()
{
	return texture;
}

void Model::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath))
		throw std::runtime_error(warn + err);

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};


			vertex.pos = { attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2] };
			vertex.texCoord = { attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };
			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back((uniqueVertices[vertex]));
		}
	}
}

void Model::loadTexture(Device* deviceInfo, CommandPool* commandPool)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	Buffer* buffer = new Buffer(deviceInfo, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	buffer->copyData(deviceInfo, pixels, 0, imageSize, 0);

	stbi_image_free(pixels);

	texture = new Image(deviceInfo, texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	texture->transitionImageLayout(deviceInfo, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	texture->copyImageFromBuffer(deviceInfo, commandPool, buffer);
	//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

	buffer->freeBuffer(deviceInfo);

	texture->generateMipmaps(deviceInfo, commandPool);
	texture->createImageView(deviceInfo, VK_IMAGE_ASPECT_COLOR_BIT);
	texture->createTextureSampler(deviceInfo);
}