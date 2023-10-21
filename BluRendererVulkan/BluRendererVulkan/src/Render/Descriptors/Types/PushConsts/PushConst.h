#include <glm/matrix.hpp>


// Position and rotation data
struct PushConstantData {
	glm::mat4 position;

	PushConstantData(glm::mat4 pos)
		: position(pos) {

	}
};