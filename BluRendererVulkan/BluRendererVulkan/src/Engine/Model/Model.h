#pragma once
#include <vector>
#include <memory>
#include "../Mesh/Mesh.h"
#include "../src/Engine/Scene/SceneUtils.h"

class Model {
public:
	Model(const SceneModel& modelPath);
    void cleanup();

private:
	
};