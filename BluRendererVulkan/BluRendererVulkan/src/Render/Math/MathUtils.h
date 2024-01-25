#pragma once
#include <glm/fwd.hpp>
#include "../../Engine/Entity/Components/TransformComponent.h"

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;

    BoundingBox() = default;
    BoundingBox(glm::vec3 min, glm::vec3 max)
        : min(min), max(max) {};

};

class MathUtils {
public:
    static glm::fvec3 CalculateForwardFromEulerAngles(const glm::fvec3& eulerAngles);
    static glm::mat4 ApplyTransformAndRotation(const glm::fvec3& transform, const glm::fvec3& eulerAngles);
    static glm::mat4 TransformToMat4(const Transform* transform);
    static bool isBoxInFrustum(glm::vec4* frustumPlanes, glm::vec4* frustumCorners, const BoundingBox& box);
	static void getFrustumPlanes(glm::mat4 mvp, glm::vec4* planes);
	static void getFrustumCorners(glm::mat4 mvp, glm::vec4* points);
};