#include "../src/Render/Device/Device.h"
#include "../src/Render/Buffer/Buffer.h"
#include <glm/matrix.hpp>

struct UniformBufferObject {
	//alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

class UBO {
public:
	UBO(Device*, VkDeviceSize, uint32_t);

	Buffer* getBuffer(const uint32_t index);

private:
	std::vector<Buffer*> buffers;
};