#pragma once
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <vector>

struct SceneLight {
	glm::vec4 lightPosition; // X,Y,Z Position	| W LightType
	glm::vec4 lightDirection; // X,Y,Z Rotation	| W LightIntensity
	glm::vec4 lightColor;

	enum LightType { Sun = 0, Directional = 1, };

	SceneLight(LightType lightType, glm::vec3 pos, glm::vec3 dir, glm::vec4 color, float intensity)
		: lightColor(color) {
		lightPosition = glm::vec4(pos, lightType);
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

	SceneModel(const char* modelPath, const char* texturePath, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, bool isDynamic, bool useGPUInstancing) 
		: modelPath(modelPath), texturePath(texturePath) {
		position = glm::vec4(pos, isDynamic ? 0 : 1);
		rotation = glm::vec4(rot, useGPUInstancing ? 0 : 1);
		scale = glm::vec4(scale, 0.0f);
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
	std::vector<SceneCamera> camera; //TODO: SUPPORT MULTIPLE CAMERAS
	//Models---------------------------
	std::vector<SceneModel> models;

	//Ligting--------------------------
	std::vector<SceneLight> lights;

	//Fog------------------------------
	glm::vec4 fogColor; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;

	SceneInfo() {
		ambientColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
		fogColor = glm::vec4(0.048f, 0.048f, 0.048f, 1.0f); // Light Gray
		fogDistances = glm::vec4(1.0f, 10.0f, 0.0f, 0.0f);
		lights.push_back(SceneLight(SceneLight::Sun, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), 1.0f));
		models.push_back(SceneModel("models / viking_room.obj", "textures / viking_room.png", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), true, false));
		camera.push_back(SceneCamera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
	}
};

class Scene {
public:
	Scene(const char* scenePath);
	
	void cleanup();

	SceneInfo* info;
};