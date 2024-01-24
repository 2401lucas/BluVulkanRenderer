#pragma once
#include <list>
#include "EntityChunk.h"

class EntityChunkManager {
public:
	uint64_t addEntityData(std::vector<BaseComponent*> data);
	std::vector<BaseComponent*> getData(uint64_t id);
	void removeData(uint64_t id);
	//TODO: Limit Chunks to 16KB & fix this
	uint32_t getSize();
private:
	std::vector<uint64_t> activeEntityIds;
	//TODO: Replace list with container that's both Non-Contiguous & has random access
	std::list<EntityChunk> entityChunks;
};