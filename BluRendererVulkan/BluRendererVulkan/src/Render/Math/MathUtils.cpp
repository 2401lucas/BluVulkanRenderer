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

glm::mat4 MathUtils::ApplyTransformAndRotation(const glm::fvec3& transform, const glm::fvec3& eulerAngles)
{
    glm::mat4 newMat = glm::mat4(1.0f);

    //newMat = glm::rotate(newMat, glm::radians(eulerAngles.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
    //newMat = glm::rotate(newMat, glm::radians(eulerAngles.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
    //newMat = glm::rotate(newMat, glm::radians(eulerAngles.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll

    newMat = glm::translate(newMat, transform);
    return newMat;
}

//TODO: Implement Rotation and Scale
glm::mat4 MathUtils::TransformToMat4(const Transform* transformData)
{
    glm::mat4 newMat = glm::mat4(1.0f);

    //newMat = glm::rotate(newMat, glm::radians(eulerAngles.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
    //newMat = glm::rotate(newMat, glm::radians(eulerAngles.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
    //newMat = glm::rotate(newMat, glm::radians(eulerAngles.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll

    newMat = glm::translate(newMat, transformData->position);
    return newMat;
}


// false if fully outside, true if inside or intersects
inline bool MathUtils::boxInFrustum(glm::vec4* frustumPlanes, glm::vec4* frustumCorners, const BoundingBox& box)
{
    using glm::dot;
    using glm::vec4;

    /*  vec4 frustumPlanes[6];
      getFrustumPlanes(proj * g_CullingView, frustumPlanes);
      vec4 frustumCorners[8];
      getFrustumCorners(proj * g_CullingView, frustumCorners);*/

    // check box outside/inside of frustum
    for (int i = 0; i < 6; i++) {
        int r = 0;
        r += (dot(frustumPlanes[i], vec4(box.min_.x, box.min_.y, box.min_.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (dot(frustumPlanes[i], vec4(box.max_.x, box.min_.y, box.min_.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (dot(frustumPlanes[i], vec4(box.min_.x, box.max_.y, box.min_.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (dot(frustumPlanes[i], vec4(box.max_.x, box.max_.y, box.min_.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (dot(frustumPlanes[i], vec4(box.min_.x, box.min_.y, box.max_.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (dot(frustumPlanes[i], vec4(box.max_.x, box.min_.y, box.max_.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (dot(frustumPlanes[i], vec4(box.min_.x, box.max_.y, box.max_.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (dot(frustumPlanes[i], vec4(box.max_.x, box.max_.y, box.max_.z, 1.0f)) < 0.0) ? 1 : 0;
        if (r == 8) return false;
    }

    // check frustum outside/inside box
    int r = 0;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x > box.max_.x) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x < box.min_.x) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y > box.max_.y) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y < box.min_.y) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z > box.max_.z) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z < box.min_.z) ? 1 : 0); if (r == 8) return false;

    return true;

}
