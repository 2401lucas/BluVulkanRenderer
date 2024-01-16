#include "Camera.h"
#include "../Math/MathUtils.h"

Camera::Camera(const glm::vec3& pos, const glm::vec3& rot, const float FOV, const float ratio, const float zNear, const float zFar)
    : position(pos), rotation(rot), fov(FOV), ratio(ratio), zNear(zNear), zFar(zFar) {
    updateViewMatrix();

    proj = glm::perspective(glm::radians(45.0f), ratio, zNear, zFar);
    proj[1][1] *= -1;
}

void Camera::updateCamera(const SceneCamera& cameraInfo)
{
    position = cameraInfo.position;
    rotation = cameraInfo.rotation;
    updateViewMatrix();
}

glm::mat4& Camera::getProjMat()
{
    return proj;
}

glm::mat4& Camera::getViewMat()
{
    return view;
}

void Camera::updateViewMatrix()
{
    view = glm::lookAt(position, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
}