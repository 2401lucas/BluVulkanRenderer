#pragma once
#include <glm/vec3.hpp>
#include "BaseComponent.h"

struct Transform : BaseComponent {
public:
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};