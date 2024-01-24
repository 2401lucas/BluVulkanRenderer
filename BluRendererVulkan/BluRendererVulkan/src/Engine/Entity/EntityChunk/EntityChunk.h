#pragma once
#include <vector>
#include <queue>
#include "../Components/BaseComponent.h"

class EntityChunk {
public:
	uint32_t addEntityData(std::vector<BaseComponent*> data);
	std::vector<BaseComponent*> getData(uint32_t id);
	void removeData(uint32_t id);
	bool isFull();
	uint32_t getSize();
private:
	std::vector<std::vector<BaseComponent*>> entityChunkData;
	std::queue<uint32_t> freeIds;
};