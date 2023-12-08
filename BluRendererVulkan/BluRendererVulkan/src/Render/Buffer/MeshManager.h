#pragma once
#include <vulkan/vulkan.h>
#include "../Mesh/Mesh.h"
#include "../Buffer/Buffer.h"

class MeshManager {
public:
	//Loads mesh, returns meshID
	int loadMesh(const char* meshPath);
	//Updates command buffer based on meshID. Models should be orginized in array based on meshID, so Instanced rendering can be used.
	void drawMesh(const VkCommandBuffer& commandBuffer, int modelID);
	
private:
	Buffer* createVertexBuffer();
	Buffer* createIndexBuffer();

	std::unordered_map<const char*, int> meshDataIDs;
	std::vector<Mesh> rawMeshData;
};