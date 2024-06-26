#pragma once

#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../VulkanResources/VulkanDevice.h"
#include "vulkan/vulkan.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <gli/gli.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "../../../libraries/GLTF/tiny_gltf.h"
#include "../../Renderer/BaseRenderer.h"

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

namespace vkglTF {
struct Node;

struct BoundingBox {
  glm::vec3 min;
  glm::vec3 max;
  bool valid = false;
  BoundingBox();
  BoundingBox(glm::vec3 min, glm::vec3 max);
  BoundingBox getAABB(glm::mat4 m);
};

struct TextureSampler {
  VkFilter magFilter;
  VkFilter minFilter;
  VkSamplerAddressMode addressModeU;
  VkSamplerAddressMode addressModeV;
  VkSamplerAddressMode addressModeW;
};

struct Texture {
  vks::VulkanDevice* device;
  VkImage image;
  VkImageLayout imageLayout;
  VkDeviceMemory deviceMemory;
  VkImageView view;
  uint32_t width, height;
  uint32_t mipLevels;
  uint32_t layerCount;
  VkDescriptorImageInfo descriptor;
  VkSampler sampler;
  void updateDescriptor();
  void destroy();
  // Load a texture from a glTF image (stored as vector of chars loaded via
  // stb_image) and generate a full mip chain for it
  void fromglTfImage(tinygltf::Image& gltfimage, std::string path,
                     TextureSampler textureSampler, vks::VulkanDevice* device,
                     VkQueue copyQueue);
};

struct Material {
  enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
  AlphaMode alphaMode = ALPHAMODE_OPAQUE;
  float alphaCutoff = 1.0f;
  float metallicFactor = 1.0f;
  float roughnessFactor = 1.0f;
  glm::vec4 baseColorFactor = glm::vec4(1.0f);
  glm::vec4 emissiveFactor = glm::vec4(0.0f);
  vkglTF::Texture* baseColorTexture;
  vkglTF::Texture* metallicRoughnessTexture;
  vkglTF::Texture* normalTexture;
  vkglTF::Texture* occlusionTexture;
  vkglTF::Texture* emissiveTexture;
  bool doubleSided = false;
  struct TexCoordSets {
    uint8_t baseColor = 0;
    uint8_t metallicRoughness = 0;
    uint8_t specularGlossiness = 0;
    uint8_t normal = 0;
    uint8_t occlusion = 0;
    uint8_t emissive = 0;
  } texCoordSets;
  struct Extension {
    vkglTF::Texture* specularGlossinessTexture;
    vkglTF::Texture* diffuseTexture;
    glm::vec4 diffuseFactor = glm::vec4(1.0f);
    glm::vec3 specularFactor = glm::vec3(0.0f);
  } extension;
  struct PbrWorkflows {
    bool metallicRoughness = true;
    bool specularGlossiness = false;
  } pbrWorkflows;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  int index = 0;
  bool unlit = false;
  float emissiveStrength = 1.0f;
};

struct Primitive {
  uint32_t firstIndex;
  uint32_t indexCount;
  uint32_t firstVertex;
  uint32_t vertexCount;
  Material& material;
  bool hasIndices;
  BoundingBox bb;
  Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount,
            Material& material);
  void setBoundingBox(glm::vec3 min, glm::vec3 max);
};

struct Mesh {
  vks::VulkanDevice* device;
  std::vector<Primitive*> primitives;
  BoundingBox bb;
  BoundingBox aabb;
  struct UniformBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDescriptorBufferInfo descriptor;
    VkDescriptorSet descriptorSet;
    void* mapped;
  } uniformBuffer;
  struct UniformBlock {
    glm::mat4 matrix;
    glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
    float jointcount{0};
  } uniformBlock;
  Mesh(vks::VulkanDevice* device, glm::mat4 matrix);
  ~Mesh();
  void setBoundingBox(glm::vec3 min, glm::vec3 max);
};

struct Skin {
  std::string name;
  Node* skeletonRoot = nullptr;
  std::vector<glm::mat4> inverseBindMatrices;
  std::vector<Node*> joints;
};

struct Node {
  Node* parent;
  uint32_t index;
  std::vector<Node*> children;
  glm::mat4 matrix;
  std::string name;
  Mesh* mesh;
  Skin* skin;
  int32_t skinIndex = -1;
  glm::vec3 translation{};
  glm::vec3 scale{1.0f};
  glm::quat rotation{};
  BoundingBox bvh;
  BoundingBox aabb;
  glm::mat4 localMatrix();
  glm::mat4 getMatrix();
  void update();
  ~Node();
};

struct AnimationChannel {
  enum PathType { TRANSLATION, ROTATION, SCALE };
  PathType path;
  Node* node;
  uint32_t samplerIndex;
};

struct AnimationSampler {
  enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
  InterpolationType interpolation;
  std::vector<float> inputs;
  std::vector<glm::vec4> outputsVec4;
};

struct Animation {
  std::string name;
  std::vector<AnimationSampler> samplers;
  std::vector<AnimationChannel> channels;
  float start = std::numeric_limits<float>::max();
  float end = std::numeric_limits<float>::min();
};

enum class VertexComponent {
  Position,
  Normal,
  UV0,
  UV1,
  Color,
  Tangent,
  Joint0,
  Weight0
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv0;
  glm::vec2 uv1;
  glm::vec4 tangent;
  glm::vec4 joint0;
  glm::vec4 weight0;
  glm::vec4 color;

  static VkVertexInputBindingDescription vertexInputBindingDescription;
  static std::vector<VkVertexInputAttributeDescription>
      vertexInputAttributeDescriptions;
  static VkPipelineVertexInputStateCreateInfo
      pipelineVertexInputStateCreateInfo;
  static VkVertexInputBindingDescription inputBindingDescription(
      uint32_t binding);
  static VkVertexInputAttributeDescription inputAttributeDescription(
      uint32_t binding, uint32_t location, VertexComponent component);
  static std::vector<VkVertexInputAttributeDescription>
  inputAttributeDescriptions(uint32_t binding,
                             const std::vector<VertexComponent> components);
  /** @brief Returns the default pipeline vertex input state create info
   * structure for the requested vertex components */
  static VkPipelineVertexInputStateCreateInfo* getPipelineVertexInputState(
      const std::vector<VertexComponent> components);
};

enum FileLoadingFlags {
  None = 0x00000000,
  PreTransformVertices = 0x00000001,
  PreMultiplyVertexColors = 0x00000002,
  FlipY = 0x00000004,
  DontLoadImages = 0x00000008
};

class Model {
 public:
  vks::VulkanDevice* device;

  struct Vertices {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory;
  } vertices;
  struct Indices {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory;
  } indices;

  glm::mat4 aabb;

  std::vector<Node*> nodes;
  std::vector<Node*> linearNodes;

  std::vector<Skin*> skins;

  std::vector<Texture> images;
  std::vector<Texture> textures;
  std::vector<TextureSampler> textureSamplers;
  std::vector<Material> materials;
  std::vector<Animation> animations;
  std::vector<std::string> extensions;

  std::string filePath;

  struct Dimensions {
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
  } dimensions;

  struct LoaderInfo {
    uint32_t* indexBuffer;
    Vertex* vertexBuffer;
    size_t indexPos = 0;
    size_t vertexPos = 0;
  };

  void destroy();
  void loadNode(vkglTF::Node* parent, const tinygltf::Node& node,
                uint32_t nodeIndex, const tinygltf::Model& model,
                LoaderInfo& loaderInfo);
  void getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model,
                    size_t& vertexCount, size_t& indexCount);
  void loadSkins(tinygltf::Model& gltfModel);
  void loadTextures(tinygltf::Model& gltfModel, VkQueue transferQueue);
  VkSamplerAddressMode getVkWrapMode(int32_t wrapMode);
  VkFilter getVkFilterMode(int32_t filterMode);
  void loadTextureSamplers(tinygltf::Model& gltfModel);
  void loadMaterials(tinygltf::Model& gltfModel);
  void loadAnimations(tinygltf::Model& gltfModel);
  void loadFromFile(std::string filename, vks::VulkanDevice* device,
                    VkQueue transferQueue,
                    uint32_t fileLoadingFlags = vkglTF::FileLoadingFlags::None);
  void drawNode(Node* node, VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);
  void calculateBoundingBox(Node* node, Node* parent);
  void getSceneDimensions();
  void updateAnimation(uint32_t index, float time);
  Node* findNode(Node* parent, uint32_t index);
  Node* nodeFromIndex(uint32_t index);
};
}  // namespace vkglTF