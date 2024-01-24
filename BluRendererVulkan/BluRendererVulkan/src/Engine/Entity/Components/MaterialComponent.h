#pragma once

#include "BaseComponent.h"

enum TextureType {
	SingleTexture = 0,
	Phong = 1,
	PBR = 2,
	Cubemap = 3,
};

struct MaterialData : BaseComponent {
public:
	/*const char* modelPath;
	const char* texturePath;*/
	TextureType textureType;
	int textureIndex;
	int materialIndex;
	int pipelineIndex;
};