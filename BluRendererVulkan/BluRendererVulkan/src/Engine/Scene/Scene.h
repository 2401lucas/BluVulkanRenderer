#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <string>
#include "../Entity/Components/MaterialComponent.h"

struct SceneLight {
	int lightType;
	glm::vec3 lightPosition;	// XYZ Position
	glm::vec3 lightRotation;	// XYZ Rotation
	glm::vec4 lightColor;
	float constant;
	float linear;
	float quad;
	float innerCutoff;
	float outerCutoff;

	//Directional Light
	SceneLight(glm::vec3 rot, glm::vec4 color) 
		: lightRotation(rot), lightColor(color) {
		lightType = 1;
		lightPosition = glm::vec3(0);
		constant = 0;
		linear = 0;
		quad = 0;
		innerCutoff = 0;
		outerCutoff = 0;
	}

	//Point Light
	SceneLight(glm::vec3 pos, glm::vec4 color, float constant, float linear, float quad)
		: lightType(lightType), lightPosition(pos), lightColor(color), constant(constant), linear(linear), quad(quad) {
		lightType = 2;
		lightRotation = glm::vec3(0);
		innerCutoff = 0;
		outerCutoff = 0;
	}

	//Spot Light
	SceneLight(glm::vec3 pos, glm::vec3 rot, glm::vec4 color, float constant, float linear, float quad, float innerCutoff, float outerCutoff)
		: lightPosition(pos), lightRotation(rot), lightColor(color), constant(constant), linear(linear), quad(quad), innerCutoff(innerCutoff), outerCutoff(outerCutoff) {
		lightType = 3;
	}
};

struct SceneCamera {
	glm::vec3 position; // X,Y,Z Position
	glm::vec3 rotation; // X,Y,Z Rotation 
	float fov;
	float ratio;
	float zNear;
	float zFar;

	SceneCamera(glm::vec3 pos, glm::vec3 rot, float fov, float ratio, float zNear, float zFar) 
	: fov(fov), ratio(ratio), zNear(zNear), zFar(zFar) {
		position = glm::vec4(pos, 0);
		rotation = glm::vec4(rot, 0);
	}
};

struct TextureInfo {
	std::string fileName;
	std::string fileType;
	TextureType type;

	TextureInfo(const std::string& fName, const std::string& fType, const TextureType& type)
		: fileName(fName), fileType(fType), type(type) {}
};

// TODO: Repack Memory
struct SceneModel {
	glm::vec4 position; // X,Y,Z Position	| W TBD
	glm::vec4 rotation; // X,Y,Z Rotation	| W TBD
	glm::vec4 scale;	// X,Y,Z Scale		| W TBD
	const char* modelPath;
	TextureInfo textureInfo;
	//This will be read in when loading models from scene file
	int shaderSetID;
	int materialIndex;

	SceneModel(const char* modelPath, TextureInfo textureInfo, const int& shaderPath, const int& materialIndex, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
		: modelPath(modelPath), textureInfo(textureInfo), shaderSetID(shaderPath), materialIndex(materialIndex) {
		position = glm::vec4(pos, 0);
		rotation = glm::vec4(rot, 0);
		this->scale = glm::vec4(scale, 0.0f);
	}

	glm::vec3 getPos() {
		return glm::vec3(position.x, position.y, position.z);
	}

	glm::vec3 getRot() {
		return glm::vec3(rotation.x, rotation.y, rotation.z);
	}

	glm::vec3 getScale() {
		return glm::vec3(scale.x, scale.y, scale.z);
	}
};

struct SceneInfo {
	//Cameras--------------------------
	std::vector<SceneCamera> cameras;
	//Models---------------------------
	std::vector<SceneModel> staticModels;
	std::vector<SceneModel> dynamicModels;
	//Lighting--------------------------
	std::vector<SceneLight> lights;

	//Fog------------------------------
	glm::vec4 fogColor; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;
};

enum shaderType
{
	NONE = 0,
	VERTEX = 1,
	FRAGMENT = 2,
	COMPUTE = 3
};

struct ShaderInfo
{
	shaderType type;
	std::string fileName;

	ShaderInfo(const shaderType& sType, const std::string& fName)
		: type(sType), fileName(fName) {}
};

struct MaterialInfo {
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;

	MaterialInfo(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) 
		: ambient(ambient), diffuse(diffuse), specular(specular), shininess(shininess) {}
};

struct SceneDependancies {
	std::vector<std::pair<ShaderInfo, ShaderInfo>> shaderPairs;
	std::vector<ShaderInfo> computeShaders;
	std::vector<TextureInfo> textures;
	std::vector<MaterialInfo> materials;
};

class Scene {
public:
	Scene(const char* scenePath);
	
	void cleanup();
	SceneInfo* getSceneInfo();
	SceneDependancies* getSceneDependancies();

private:
	SceneInfo* info;
	SceneDependancies* sceneDependancies;
};