#pragma once
#include <vector>
#include "../Mesh/Mesh.h"
#include "../src/Engine/Scene/SceneUtils.h"

class Model {
public:
	Model(const SceneModel& modelPath, uint32_t textureIndex);
    void cleanup();

	Mesh* getMesh();
	uint32_t getTextureIndex();
	glm::vec3& getPosition();
private:
	Mesh* mesh;
	uint32_t textureIndex;

	//TODO: Inherit this 
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};