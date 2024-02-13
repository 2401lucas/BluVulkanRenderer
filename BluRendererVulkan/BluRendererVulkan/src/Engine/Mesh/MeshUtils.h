#pragma once
#include <vulkan/vulkan_core.h>

#include <array>
#include <glm/gtx/hash.hpp>
#include <string>
#include <vector>

#include "../../Render/Math/MathUtils.h"
struct InstanceData {
  glm::mat4 mvp;
  uint32_t texIndex{0};
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec2 texCoord;

  static std::array<VkVertexInputBindingDescription, 2>
  getBindingDescription() {
    std::array<VkVertexInputBindingDescription, 2> bindingDescription{};
    bindingDescription[0].binding = 0;
    bindingDescription[0].stride = sizeof(Vertex);
    bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindingDescription[1].binding = 1;
    bindingDescription[1].stride = sizeof(InstanceData);
    bindingDescription[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 9>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 9> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, normal);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

    attributeDescriptions[4].binding = 1;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[4].offset = sizeof(glm::vec4) * 0;

    attributeDescriptions[5].binding = 1;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[5].offset = sizeof(glm::vec4) * 1;

    attributeDescriptions[6].binding = 1;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[6].offset = sizeof(glm::vec4) * 2;

    attributeDescriptions[7].binding = 1;
    attributeDescriptions[7].location = 7;
    attributeDescriptions[7].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[7].offset = sizeof(glm::vec4) * 3;

    attributeDescriptions[8].binding = 1;
    attributeDescriptions[8].location = 8;
    attributeDescriptions[8].format = VK_FORMAT_R32_SINT;
    attributeDescriptions[8].offset = offsetof(InstanceData, texIndex);

    return attributeDescriptions;
  }

  bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color &&
           texCoord == other.texCoord && normal == other.normal;
  }
};

namespace std {
template <>
struct hash<Vertex> {
  size_t operator()(Vertex const& vertex) const {
    return ((((hash<glm::vec3>()(vertex.pos) ^
               (hash<glm::vec3>()(vertex.color) << 1) >> 1) ^
              (hash<glm::vec2>()(vertex.texCoord) << 1))) >>
            1) ^
           hash<glm::vec3>()(vertex.normal) << 1;
  }
};
}  // namespace std

struct MeshData {
  std::string meshPath;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  BoundingBox boundingBox;
};

class MeshUtils {
 public:
  static void getMeshDataFromPath(MeshData* meshData);
};