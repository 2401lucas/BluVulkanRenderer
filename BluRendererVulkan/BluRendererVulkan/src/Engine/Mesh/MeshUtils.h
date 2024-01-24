#pragma once
#include <vector>
#include "../Entity/Components/MeshRendererComponent.h"
#include <glm/gtx/hash.hpp>


namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1))) >> 1) ^ hash<glm::vec3>()(vertex.normal) << 1;
        }
    };
}

class MeshUtils {
public:
	static void getMeshDataFromPath(const char* path, MeshRenderer* mr);
};