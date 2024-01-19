#pragma once
#include <cstdint>

class Entity {
public:
	uint32_t getID();

private:
	uint32_t archetypeID;
};