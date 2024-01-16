#pragma once
#include <glm/vec3.hpp>
#include "BaseComponent.h"

class TransformComponent : BaseComponent {
public:
	void setPositionAndRotation(glm::vec3 position, glm::vec3 rotation);
	void setPosition(glm::vec3 position);
	void setRotation(glm::vec3 rotation);
	void setScale(glm::vec3 position, glm::vec3 rotation);
	void lookAt(glm::vec3 position);

	glm::vec3 getPosition();
	glm::vec3 getRotation();
	glm::vec3 getScale();

private:
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};