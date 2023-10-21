#include "Camera.h"
#include "../src/Render/Math/MathUtils.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

Camera::Camera(const glm::fvec3& pos, const glm::fvec3& rotation, const float FOV, const float ratio, const float zNear, const float zFar)
	: posistion(pos), rotation(rotation), fov(FOV), ratio(ratio), zNear(zNear), zFar(zFar) {
	
    view = glm::lookAt(pos, MathUtils::CalculateForwardFromEulerAngles(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    
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