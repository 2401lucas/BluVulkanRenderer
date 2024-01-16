#include "MeshUtils.h"

std::vector<Vertex> MeshUtils::getVerticesFromModels(std::vector<Model*> models)
{
	std::vector<Vertex> vertices;

	for (Model* model : models) {
		vertices.insert(vertices.end(), model->getMesh()->getVertices().begin(), model->getMesh()->getVertices().end());
	}

	return vertices;
}

std::vector<uint32_t> MeshUtils::getIndicesFromModels(std::vector<Model*> models)
{
	std::vector<uint32_t> indices;

	for (Model* model : models) {
		indices.insert(indices.end(), model->getMesh()->getIndices().begin(), model->getMesh()->getIndices().end());
	}

	return indices;
}
