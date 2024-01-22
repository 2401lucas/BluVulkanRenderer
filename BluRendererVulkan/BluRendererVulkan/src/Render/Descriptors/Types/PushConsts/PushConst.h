#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

struct PushConstantData {
	glm::vec4 index; // X: Texture Index Y: Object Index

	PushConstantData(float modelIndex, glm::vec3 matData) {
		index = glm::vec4(modelIndex, matData);
	}
};