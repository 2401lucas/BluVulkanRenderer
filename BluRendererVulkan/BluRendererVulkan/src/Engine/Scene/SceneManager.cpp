#include "SceneManager.h"

void SceneManager::cleanup()
{
	if (scene != nullptr) {
		scene->cleanup();
		delete scene;
	}
}

//TODO: When changing scenes some UI should be in place while scene loads
// Multiple Scenes should also be able to be active, IE: Game Scene + UI Scene
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