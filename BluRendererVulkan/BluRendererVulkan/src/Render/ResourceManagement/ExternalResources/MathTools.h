#pragma once
namespace math {
namespace random {
int randomRange(int& min, int& max);
float randomRange(float& min, float& max);

}  // namespace random

namespace matrix {}
namespace linear {
float lerp(float min, float max, float mix);
}
}  // namespace math