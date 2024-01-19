#include <vector>
#include <array>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include "../Entity/Components/MeshRendererComponent.h"

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1))) >> 1) ^ hash<glm::vec3>()(vertex.normal) << 1;
        }
    };
}

class Mesh {
public: 
	Mesh(const char* path);
    void cleanup();

	std::vector<Vertex>& getVertices();
	std::vector<uint32_t>& getIndices();
	
private:
    std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};