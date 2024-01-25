#pragma once
#include <vector>
#include "../../Engine/Mesh/MeshUtils.h"
#include "../../Engine/Entity/Components/MeshRendererComponent.h"

struct RenderModelCreateData {
	MeshRenderer* meshRenderer;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};