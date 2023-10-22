#include "MathUtils.h"
#include <glm/matrix.hpp>
#include <glm/ext/matrix_transform.hpp>

glm::fvec3 MathUtils::CalculateForwardFromEulerAngles(const glm::fvec3& eulerAngles) {
    glm::mat4 rotationMatrix = glm::mat4(1.0f);

    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(eulerAngles.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(eulerAngles.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(eulerAngles.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll

    // The forward direction is the negative Z-axis
    glm::fvec3 forward = -glm::fvec3(rotationMatrix * glm::fvec4(0.0f, 0.0f, 1.0f, 0.0f));

    return forward;
}

glm::mat4 MathUtils::ApplyTransformAndRotation(const glm::fvec3& transform, const glm::fvec3& eulerAngles, const glm::mat4& mat)
{
    glm::mat4 newMat = mat;

    newMat = glm::rotate(newMat, glm::radians(eulerAngles.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
    newMat = glm::rotate(newMat, glm::radians(eulerAngles.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
    newMat = glm::rotate(newMat, glm::radians(eulerAngles.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll

    //newMat = glm::translate(newMat, transform);
    return newMat;
}
