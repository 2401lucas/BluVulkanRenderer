#pragma once
#include <vector>
#include "../../Engine/Mesh/MeshUtils.h"

struct RenderModelCreateData {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};