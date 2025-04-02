#ifndef MODEL_H
#define MODEL_H
namespace blu::game::components {
struct Model {
  uint32_t model_index;
  blu::game::components::Transform transform;
};
}  // namespace blu::game::components
#endif