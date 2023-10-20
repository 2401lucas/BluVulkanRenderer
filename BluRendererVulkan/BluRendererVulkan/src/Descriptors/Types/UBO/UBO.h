#include "../src/Device/Device.h"
#include "../../../Buffer/Buffer.h"

class UBO {
public:
	UBO(Device*, VkDeviceSize, uint32_t);


	Buffer* getBuffer(const uint32_t index);

private:
	std::vector<Buffer*> buffers;
};