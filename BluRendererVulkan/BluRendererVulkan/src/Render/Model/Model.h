#pragma once
#include <vector>
#include "../Mesh/Mesh.h"
#include "../src/Engine/Scene/SceneUtils.h"

class Model {
public:
	Model(const SceneModel& modelPath, uint32_t textureIndex, uint32_t materialIndex);
    void cleanup();

	Mesh* getMesh();
	uint32_t getTextureIndex();
	uint32_t getMaterialIndex();
	glm::vec3& getPosition();
private:
	Mesh* mesh;
	uint32_t textureIndex;
	uint32_t materialIndex;

	//TODO: Inherit this 
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};