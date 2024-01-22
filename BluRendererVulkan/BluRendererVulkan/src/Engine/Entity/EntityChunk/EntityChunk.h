#pragma once
#include <vector>
#include <queue>

class EntityChunk {
public:
	uint32_t addEntityData(void* data);
	void* getData(uint32_t id);
	void removeData(uint32_t id);
	bool isFull();
	uint32_t getSize();
private:
	std::vector<void*> entityChunkData;
	std::queue<uint32_t> freeIds;
};