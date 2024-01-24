#include "EntityChunkManager.h"

uint64_t EntityChunkManager::addEntityData(std::vector<BaseComponent*> data)
{
	for (std::list<EntityChunk>::iterator it = entityChunks.begin(); it != entityChunks.end(); it++)
	{
		if (!it->isFull()) {
			return ((uint64_t)std::distance(entityChunks.begin(), it) << 32) | it->addEntityData(data);
		}
	}

	entityChunks.push_back(EntityChunk());
	uint64_t id = ((uint64_t)entityChunks.size() << 32) | entityChunks.back().addEntityData(data);
	return id;
}

std::vector<BaseComponent*> EntityChunkManager::getData(uint64_t id)
{
	uint32_t chunkId = (uint32_t)(id >> 32);
	uint32_t dataId = (uint32_t)(id);
	
	std::list<EntityChunk>::iterator it = entityChunks.begin();
	std::advance(it, chunkId);
	
	return it->getData(dataId);
}

void EntityChunkManager::removeData(uint64_t id)
{
	uint32_t chunkId = (uint32_t)(id >> 32);
	uint32_t dataId = (uint32_t)(id);

	std::list<EntityChunk>::iterator it = entityChunks.begin();
	std::advance(it, chunkId);

	it->removeData(dataId);
}

uint32_t EntityChunkManager::getSize()
{
	std::list<EntityChunk>::iterator it = entityChunks.begin();
	return it->getSize();
}
