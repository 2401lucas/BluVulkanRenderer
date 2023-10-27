#pragma once
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <string>

struct SceneLight {
	glm::vec4 lightPosition; // X,Y,Z Position	| W 
	glm::vec4 lightDirection; // X,Y,Z Rotation	| W LightIntensity
	glm::vec4 lightColor;

	SceneLight(glm::vec3 pos, glm::vec3 dir, glm::vec4 color, float intensity)
		: lightColor(color) {
		lightPosition = glm::vec4(pos, 0);
		lightDirection = glm::vec4(dir, intensity);
	}
};

struct SceneCamera {
	glm::vec4 position; // X,Y,Z Position	| W 
	glm::vec4 rotation; // X,Y,Z Rotation	| W 

	SceneCamera(glm::vec3 pos, glm::vec3 rot) {
		position = glm::vec4(pos, 0);
		rotation = glm::vec4(rot, 0);
	}
};

// TODO: Repack Memory
struct SceneModel {
	const char* modelPath;
	const char* texturePath;
	glm::vec4 position; // X,Y,Z Position	| W isDynamic
	glm::vec4 rotation; // X,Y,Z Rotation	| W useGPUInstancing
	glm::vec4 scale;	// X,Y,Z Scale		| W TBD

	SceneModel(const char* modelPath, const char* texturePath, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, bool useGPUInstancing) 
		: modelPath(modelPath), texturePath(texturePath) {
		position = glm::vec4(pos, 0);
		rotation = glm::vec4(rot, useGPUInstancing ? 0 : 1);
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

	bool isDynamic() {
		return position.w == 0;
	}

	bool useGPUInstancing() {
		return rotation.w == 0;
	}
};

struct SceneInfo {
	//Cameras--------------------------
	std::vector<SceneCamera> cameras;
	//Models---------------------------
	std::vector<SceneModel> staticModels;
	std::vector<SceneModel> dynamicModels;
	//Ligting--------------------------
	std::vector<SceneLight> directionalLights;
	std::vector<SceneLight> spotLights;
	std::vector<SceneLight> pointLights;

	//Fog------------------------------
	glm::vec4 fogColor; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;

	SceneInfo() {
		ambientColor = glm::vec4(0.6, 0.5, 0.0, 0.2);
		fogColor = glm::vec4(0.048f, 0.048f, 0.048f, 1.0f); // Light Gray
		fogDistances = glm::vec4(1.0f, 10.0f, 0.0f, 0.0f);
		directionalLights.push_back(SceneLight(glm::vec3(0.0f), glm::vec3(0.0, 0.5, 0.5), glm::vec4(1.0, 1.0, 1.0, 1), 1.0f));
		dynamicModels.push_back(SceneModel("models/viking_room.obj", "textures/temp.png", glm::vec3(-2.0f, -2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), false));
		dynamicModels.push_back(SceneModel("models/viking_room.obj", "textures/viking_room.png", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), false));
		dynamicModels.push_back(SceneModel("models/viking_room.obj", "textures/viking_room.png", glm::vec3(-4.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), false));
		dynamicModels.push_back(SceneModel("models/viking_room.obj", "textures/viking_room.png", glm::vec3(0.0f, -4.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), false));
		cameras.push_back(SceneCamera(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
	}
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
	std::string fileName;

	MaterialInfo(const std::string& fName)
		: fileName(fName) {}
};

class Scene {
public:
	Scene(const char* scenePath);
	
	void cleanup();
	SceneInfo* getSceneInfo();

private:
	SceneInfo* info;
};