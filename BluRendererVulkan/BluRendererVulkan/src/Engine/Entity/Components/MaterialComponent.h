#pragma once
enum TextureType {
	SingleTexture,
	Phong,
	PBR,
	Cubemap,
};

struct Material {
public:
	/*const char* modelPath;
	const char* texturePath;*/
	TextureType textureType;
	int textureIndex;
	int materialIndex;
	int pipelineIndex;
};