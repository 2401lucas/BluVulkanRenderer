#pragma once
#include "../src/Engine/Entity/Components/MeshRendererComponent.h"
#include "../src/Engine/Entity/Components/TransformComponent.h"
#include "../src/Engine/Entity/Components/MaterialComponent.h"

struct RenderModelData {
	MeshRenderer meshRenderData;
	glm::vec3 materialData; // getTextureType(), getTextureIndex(), getMaterialIndex()
	glm::mat4 modelTransform;
};