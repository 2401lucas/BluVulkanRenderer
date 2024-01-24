#include "CameraSystem.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

void CameraSystem::updateCamera(Camera* cameraData, const Transform* transformData)
{
	//Proj doesn't need to be updated every frame
	cameraData->proj = glm::perspective(glm::radians(45.0f), cameraData->ratio, cameraData->zNear, cameraData->zFar);
	cameraData->proj[1][1] *= -1;
	cameraData->view = glm::lookAt(transformData->position, transformData->rotation, glm::vec3(0.0f, 0.0f, 1.0f));
}