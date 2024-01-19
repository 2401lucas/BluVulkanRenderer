#include "EntityArchetypes.h"

bool EntityArchetypes::checkComponents(uint32_t componentTypes)
{
	return this->componentsTypes == componentTypes;
}
