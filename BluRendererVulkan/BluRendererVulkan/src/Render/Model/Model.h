#pragma once
#include <vector>
#include "../Mesh/Mesh.h"

class Model {
public:
	Model(const char* modelPath, uint32_t textureIndex);
    void cleanup();

	Mesh* getMesh();
	uint32_t getTextureIndex();
private:
	Mesh* mesh;
	uint32_t textureIndex;
};