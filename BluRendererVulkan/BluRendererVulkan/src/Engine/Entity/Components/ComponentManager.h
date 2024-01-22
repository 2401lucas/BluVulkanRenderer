//OUTDATED - 2024/01/19
//This is a generic component manager for an ECS system that stores similar components. The current ECS design stores all of an entities data together, prefering to iterate of entities than components;

#include <vector>
#include <queue>

template <typename T>
class ComponentManager {
public:
	ComponentManager();

	uint32_t add();
	void remove(uint32_t id);
	T& get(uint32_t id);

	std::vector<T> components;
	std::queue<uint32_t> freeIds;
};

template<typename T>
inline ComponentManager<T>::ComponentManager()
{
	components.reserve(64);
}

template<typename T>
inline uint32_t ComponentManager<T>::add()
{
	if (!freeIds.empty())
	{
		auto id = freeIds.front();
		freeIds.pop();
		components.at(id) = T();
		return id;
	}
	else
	{
		auto id = components.size();
		components.push_back(T());
		return id;
	}
}

template<typename T>
inline void ComponentManager<T>::remove(uint32_t id)
{
	freeIds.push(id);
}

template<typename T>
inline T& ComponentManager<T>::get(uint32_t id)
{
	return components.at(id);
}
