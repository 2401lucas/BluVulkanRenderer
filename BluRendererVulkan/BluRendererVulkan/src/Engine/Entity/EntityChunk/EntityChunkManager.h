#pragma once
#include <list>
#include "EntityChunk.h"

class EntityChunkManager {
public:
	uint64_t addEntityData(void* data);
	void* getData(uint64_t id);
	void removeData(uint64_t id);
	//TODO: Limit Chunks to 16KB & fix this
	uint32_t getSize();
private:
	std::vector<uint64_t> activeEntityIds;
	//TODO: Replace list with container that's both Non-Contiguous & has random access
	std::list<EntityChunk> entityChunks;
};