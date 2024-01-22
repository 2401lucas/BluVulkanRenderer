#include <glm/fwd.hpp>
#include "../../Engine/Entity/Components/TransformComponent.h"

class MathUtils {
public:
    static glm::fvec3 CalculateForwardFromEulerAngles(const glm::fvec3& eulerAngles);
    static glm::mat4 ApplyTransformAndRotation(const glm::fvec3& transform, const glm::fvec3& eulerAngles);
    static glm::mat4 TransformToMat4(const Transform* transform);
};