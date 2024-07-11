#include "MathTools.h"

#include <random>

namespace math {
namespace random {
int randomRange(int min, int max) {
  std::random_device rd;      // seed for PRNG
  std::mt19937 mt_eng(rd());  // mersenne-twister engine initialised with seed

  std::uniform_int_distribution<> dist(min, max);

  return dist(mt_eng);
}
float randomRange(float min, float max) {
  std::random_device rd;      // seed for PRNG
  std::mt19937 mt_eng(rd());  // mersenne-twister engine initialised with seed

  std::uniform_real_distribution<> dist(min, max);

  return dist(mt_eng);
}
}  // namespace random

namespace linear {
float lerp(float min, float max, float mix) { return min + mix * (max - min); }
}  // namespace linear
}  // namespace math
