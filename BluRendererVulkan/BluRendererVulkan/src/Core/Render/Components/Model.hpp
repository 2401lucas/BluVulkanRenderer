#include "Material.hpp"
#include "Mesh.hpp"

namespace core_internal::rendering::components {
struct GPUModelData {};
}  // namespace core_internal::rendering::components

namespace core::engine::components {
struct Model {
  bool isStatic; // Does this matter?
  bool canCastShadows;
  bool receiveShadows;
  std::string filePath;
  Mesh mesh;
  Material material;
};
}  // namespace core::engine::components