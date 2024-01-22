#include "EntityChunk.h"

uint32_t EntityChunk::addEntityData(void* data)
{
	if (!freeIds.empty()) {
		auto id = freeIds.front();
		freeIds.pop();
		entityChunkData.at(id) = data;
		return id;
	}

	entityChunkData.push_back(data);
	return (entityChunkData.size() - 1);
}

void* EntityChunk::getData(uint32_t id)
{
	return entityChunkData.at(id);
}

void EntityChunk::removeData(uint32_t id)
{
	freeIds.push(id);
}

//TODO: Implement size limit (16KB)
bool EntityChunk::isFull()
{
	return false;
}

uint32_t EntityChunk::getSize()
{
	return entityChunkData.size();
}
