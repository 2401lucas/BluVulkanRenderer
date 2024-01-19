#include "EntityChunkManager.h"

uint64_t EntityChunkManager::addEntityData(void* data)
{
	for (std::list<EntityChunk>::iterator it = entityChunks.begin(), int i = 0; it != entityChunks.end(); i++)
	{
		if (!it->isFull()) {
			return	((uint64_t)i << 32) | it->addEntityData(data);
		}
	}

	entityChunks.push_back(EntityChunk());
	return ((uint64_t)entityChunks.size() << 32) | entityChunks.back().addEntityData(data);
}

void* EntityChunkManager::getData(uint64_t id)
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