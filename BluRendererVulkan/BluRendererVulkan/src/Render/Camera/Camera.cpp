#include "Camera.h"
#include "../Math/MathUtils.h"

Camera::Camera(const glm::vec3& pos, const glm::vec3& rotation, const float FOV, const float ratio, const float zNear, const float zFar)
    : position(pos), rotation(rotation), fov(FOV), ratio(ratio), zNear(zNear), zFar(zFar) {
    view = glm::lookAt(pos, rotation, glm::vec3(0.0f, 0.0f, 1.0f));

    proj = glm::perspective(glm::radians(45.0f), ratio, zNear, zFar);
    proj[1][1] *= -1;
}

glm::mat4& Camera::getProjMat()
{
    return proj;
}

glm::mat4& Camera::getViewMat()
{
    return view;
}