#ifndef MATERIAL_H
#define MATERIAL_H

#include <eastl/string.h>

namespace blu::core::components {
class Material {
 public:
  struct TextureInfo {
    eastl::string filepath;
    int index = -1;
  };

  Material();
  ~Material();

  TextureInfo& GetBaseColorTextureInfo() { return base_color_; };
  TextureInfo& GetNormalTextureInfo() { return normal_; };
  TextureInfo& GetEmissionTextureInfo() { return emission_color_; };
  TextureInfo& GetMetalnessTextureInfo() { return metalness_; };
  TextureInfo& GetDiffuseRoughnessTextureInfo() { return diffuse_roughness_; };
  TextureInfo& GetAmbientOcclusionTextureInfo() { return ambient_occlusion_; };

 private:
  TextureInfo base_color_;
  TextureInfo normal_;
  TextureInfo emission_color_;
  TextureInfo metalness_;
  TextureInfo diffuse_roughness_;
  TextureInfo ambient_occlusion_;
};
}  // namespace blu::core::components

#endif