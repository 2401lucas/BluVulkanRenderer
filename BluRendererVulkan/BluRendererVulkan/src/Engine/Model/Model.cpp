#include "Model.h"

Model::Model(const SceneModel& modelInfo) {

}

void Model::cleanup() {
    for (auto component : components) {
        //component->cleanup();
        delete component;
    }
}

//The ID of components is derived from the order of the components given when creating an Entity
BaseComponent* Model::getComponent(uint32_t componentID)
{
	return components.at(componentID);
}
