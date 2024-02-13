#pragma once
#include <vector>
#include "../../Engine/Mesh/MeshUtils.h"

struct RenderMeshCreateData {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};