#pragma once
#include "../Components/CameraComponent.h"
#include "../Components/TransformComponent.h"

class CameraSystem {
public:
	static void updateCamera(Camera* cameraData, const Transform* transformData);
};