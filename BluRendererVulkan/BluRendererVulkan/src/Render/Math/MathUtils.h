#include <glm/fwd.hpp>

class MathUtils {
public:
    static glm::fvec3 CalculateForwardFromEulerAngles(const glm::fvec3& eulerAngles);
    static glm::mat4 ApplyTransformAndRotation(const glm::fvec3& transform, const glm::fvec3& eulerAngles, const glm::mat4& mat);
};