#include "MeshManager.h"

int MeshManager::loadMesh(const char* meshPath)
{
	if (meshDataIDs.count(meshPath) == (size_t)0) {
		rawMeshData.push_back(Mesh(meshPath));
		meshDataIDs[meshPath] = rawMeshData.size() - 1;
	}

	return meshDataIDs[meshPath];
}

void MeshManager::drawMesh(const VkCommandBuffer& commandBuffer, int modelID)
{
	VkDeviceSize offsets[] = { 0 };

	/*auto vertBuff = pipelineVertexBuffers[pipelineIndex]->getBuffer();
	auto indBuff = pipelineVertexBuffers[pipelineIndex]->getBuffer();
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertBuff, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indBuff, 0, VK_INDEX_TYPE_UINT32);*/
}