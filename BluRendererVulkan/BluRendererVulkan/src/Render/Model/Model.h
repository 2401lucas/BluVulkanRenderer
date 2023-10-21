#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>
#include "../Device/Device.h"
#include "../Image/Image.h"
#include "../Command/CommandPool.h"
#include "../Mesh/Mesh.h"

struct ModelCreateInfo {
	const char* modelPath;
	const char* texturePath;
	const glm::fvec3 pos;
	const glm::fvec3 rot;

	ModelCreateInfo(const char* modelPath, const char* texturePath, const glm::fvec3 pos, const glm::fvec3 rot)
		: modelPath(modelPath), texturePath(texturePath), pos(pos), rot(rot)
	{

	}
};

class Model {
public:
	Model(Device* deviceInfo, CommandPool* commandPool, ModelCreateInfo createInfo);
    void cleanup(Device* deviceInfo);

	Mesh* getMesh();
	Image* getTexture();
private:
	Mesh* mesh;
	Image* texture;
};