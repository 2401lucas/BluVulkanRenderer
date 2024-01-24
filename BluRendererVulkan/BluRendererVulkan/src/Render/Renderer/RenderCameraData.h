#pragma once
#include <glm/mat4x4.hpp>

struct RenderCameraData {
public:
	glm::mat4 viewMat;
	glm::mat4 projMat;
	glm::vec3 position;
};