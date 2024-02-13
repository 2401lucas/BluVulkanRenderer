#pragma once
#include "Scene.h"

class SceneManager {
public:
	void cleanup();
	void loadScene(const char* path);
	const SceneInfo* getSceneInfo();
	const SceneDependancies* getSceneDependancies();
private:

public:

private:
	Scene* scene;
};