#include "MeshUtils.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <algorithm>

void MeshUtils::getMeshDataFromPath(MeshData* meshData)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, meshData->meshPath))
		throw std::runtime_error(warn + err);

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = { attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2] };
			vertex.texCoord = { attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };
			vertex.color = { 1.0f, 1.0f, 1.0f };
			vertex.normal = { attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1], attrib.normals[3 * index.normal_index + 2] };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(meshData->vertices.size());
				meshData->vertices.push_back(vertex);
			}

			meshData->indices.push_back((uniqueVertices[vertex]));
		}
	}

	//Calculate Bounding box
	auto xExtremes = std::minmax_element(meshData->vertices.begin(), meshData->vertices.end(),
		[](const Vertex& lhs, const Vertex& rhs) {
			return lhs.pos.x < rhs.pos.x;
		});

	auto yExtremes = std::minmax_element(meshData->vertices.begin(), meshData->vertices.end(),
		[](const Vertex& lhs, const Vertex& rhs) {
			return lhs.pos.y < rhs.pos.y;
		});

	auto zExtremes = std::minmax_element(meshData->vertices.begin(), meshData->vertices.end(),
		[](const Vertex& lhs, const Vertex& rhs) {
			return lhs.pos.z < rhs.pos.z;
		});

	glm::vec3 upperLeft = glm::vec3((xExtremes.first->pos, yExtremes.first->pos, zExtremes.first->pos));
	glm::vec3 lowerRight = glm::vec3((xExtremes.second->pos, yExtremes.second->pos, zExtremes.second->pos));
	meshData->boundingBox = BoundingBox(upperLeft, lowerRight);
}