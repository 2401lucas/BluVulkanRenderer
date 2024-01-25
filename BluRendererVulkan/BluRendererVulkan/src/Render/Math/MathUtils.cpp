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

void MathUtils::getFrustumPlanes(glm::mat4 mvp, glm::vec4* planes)
{
    mvp = glm::transpose(mvp);
    planes[0] = glm::vec4(mvp[3] + mvp[0]); // left
    planes[1] = glm::vec4(mvp[3] - mvp[0]); // right
    planes[2] = glm::vec4(mvp[3] + mvp[1]); // bottom
    planes[3] = glm::vec4(mvp[3] - mvp[1]); // top
    planes[4] = glm::vec4(mvp[3] + mvp[2]); // near
    planes[5] = glm::vec4(mvp[3] - mvp[2]); // far
}

void MathUtils::getFrustumCorners(glm::mat4 mvp, glm::vec4* points)
{
    const glm::vec4 corners[] = {
         glm::vec4(-1, -1, -1, 1), glm::vec4(1, -1, -1, 1),
         glm::vec4(1,  1, -1, 1),  glm::vec4(-1,  1, -1, 1),
         glm::vec4(-1, -1,  1, 1), glm::vec4(1, -1,  1, 1),
         glm::vec4(1,  1,  1, 1),  glm::vec4(-1,  1,  1, 1)
    };

    const glm::mat4 invMVP = glm::inverse(mvp);

    for (int i = 0; i != 8; i++) {
        const glm::vec4 q = invMVP * corners[i];
        points[i] = q / q.w;
    }
}


// false if fully outside, true if inside or intersects
bool MathUtils::isBoxInFrustum(glm::vec4* frustumPlanes, glm::vec4* frustumCorners, const BoundingBox& box)
{
    // check box outside/inside of frustum
    for (int i = 0; i < 6; i++) {
        int r = 0;
        r += (glm::dot(frustumPlanes[i], glm::vec4(box.min.x, box.min.y, box.min.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (glm::dot(frustumPlanes[i], glm::vec4(box.max.x, box.min.y, box.min.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (glm::dot(frustumPlanes[i], glm::vec4(box.min.x, box.max.y, box.min.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (glm::dot(frustumPlanes[i], glm::vec4(box.max.x, box.max.y, box.min.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (glm::dot(frustumPlanes[i], glm::vec4(box.min.x, box.min.y, box.max.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (glm::dot(frustumPlanes[i], glm::vec4(box.max.x, box.min.y, box.max.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (glm::dot(frustumPlanes[i], glm::vec4(box.min.x, box.max.y, box.max.z, 1.0f)) < 0.0) ? 1 : 0;
        r += (glm::dot(frustumPlanes[i], glm::vec4(box.max.x, box.max.y, box.max.z, 1.0f)) < 0.0) ? 1 : 0;
        if (r == 8) return false;
    }

    // check frustum outside/inside box
    int r = 0;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x > box.max.x) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x < box.min.x) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y > box.max.y) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y < box.min.y) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z > box.max.z) ? 1 : 0); if (r == 8) return false;
    r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z < box.min.z) ? 1 : 0); if (r == 8) return false;

    return true;

}
