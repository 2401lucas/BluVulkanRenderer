#include "SceneManager.h"

void SceneManager::cleanup()
{
	if (scene != nullptr) {
		scene->cleanup();
		delete scene;
	}
}

void SceneManager::loadScene(const char* path)
{
	cleanup();

	scene = new Scene(path);
}

const SceneInfo* SceneManager::getSceneInfo()
{
	return scene->getSceneInfo();
}

const SceneDependancies* SceneManager::getSceneDependancies()
{
	return scene->getSceneDependancies();
}
