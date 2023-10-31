#pragma once
#include <vector>
#include "Scene.h"

struct BuildDependancies {
	std::vector<ShaderInfo> shaders;
	std::vector<TextureInfo> textures;
	std::vector<MaterialInfo> materials;
};

class SceneUtils {
public:
	SceneUtils() = delete;

	static BuildDependancies getBuildDependencies();
};