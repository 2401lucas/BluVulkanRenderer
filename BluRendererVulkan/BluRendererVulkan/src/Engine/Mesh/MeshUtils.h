#pragma once

#include "../Model/Model.h"

class MeshUtils {
public:
	static std::vector<Vertex> getVerticesFromModels(std::vector<Model*> models);
	static std::vector<uint32_t> getIndicesFromModels(std::vector<Model*> models);
};