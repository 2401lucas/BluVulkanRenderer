#pragma once
#include <vector>
#include "../Model/Model.h"

//Model Settings:
//Static Models
//Scene Persistant models(DDOL)

class ModelManager {
public:
	ModelManager();
	void cleanup();

	//Models
	uint32_t addModel(const SceneModel& modelCreateInfos);
	void deleteAllModels();
	void deleteModel(Model* model);
	std::list<Model*> getModels();

private:
	std::list<Model*> models;
};