#include "../src/Render/Device/Device.h"
#include "../src/Render/Buffer/Buffer.h"
#include <glm/matrix.hpp>

struct GPUCameraData {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct GPUSceneData {
	glm::vec4 cameraPosition; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; //w for sun power
	glm::vec4 sunlightColor;
};


class UBO {
public:
	UBO(Device*, VkDeviceSize, uint32_t);

	Buffer* getBuffer(const uint32_t index);

private:
	std::vector<Buffer*> buffers;
};