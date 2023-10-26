#include <glm/matrix.hpp>

struct PushConstantData {
	glm::vec4 index; // X: Texture Index Y: Object Index

	PushConstantData(glm::vec4 index)
		: index(index) {

	}
};