#pragma once
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <string>

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

	SceneCamera(glm::vec3 pos, glm::vec3 rot) {
		position = glm::vec4(pos, 0);
		rotation = glm::vec4(rot, 0);
	}
};

// TODO: Repack Memory
struct SceneModel {
	const char* modelPath;
	const char* texturePath;
	int materialIndex;
	glm::vec4 position; // X,Y,Z Position	| W isDynamic
	glm::vec4 rotation; // X,Y,Z Rotation	| W useGPUInstancing
	glm::vec4 scale;	// X,Y,Z Scale		| W TBD

	SceneModel(const char* modelPath, const char* texturePath, int materialIndex, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, bool useGPUInstancing)
		: modelPath(modelPath), texturePath(texturePath), materialIndex(materialIndex) {
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
	//Lighting--------------------------
	std::vector<SceneLight> lights;

	//Fog------------------------------
	glm::vec4 fogColor; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;

	SceneInfo() {
		ambientColor = glm::vec4(0.6, 0.5, 0.1, 0.1);
		fogColor = glm::vec4(0.048f, 0.048f, 0.048f, 1.0f); // Light Gray
		fogDistances = glm::vec4(1.0f, 10.0f, 0.0f, 0.0f);
		//lights.push_back(SceneLight(1, glm::vec3(0.0f), glm::vec3(0.0, 0.5, 0.5), glm::vec4(1.0, 1.0, 1.0, 1)));
		//lights.push_back(SceneLight(1, glm::vec3(0.0f), glm::vec3(0.5, 0.5, 0), glm::vec4(1.0, 1.0, 1.0, 1)));
		//lights.push_back(SceneLight(glm::vec3(-5.0f, -5.0f, 3.0f), glm::vec4(1.0, 1.0, 1.0, 5), 1, 0.09f, 0.032f));
		lights.push_back(SceneLight(glm::vec3(10.0f, 10.0f, 0.0f), glm::vec3(0.45, 0.45, 0.1), glm::vec4(1.0, 1.0, 1.0, 1), 1, 0.045f, 0.0075f, 1, 1.5));
		dynamicModels.push_back(SceneModel("models/cube.obj", "textures/container.png", 1, glm::vec3(-1.0f, -1.0f, -0.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), false));
		//dynamicModels.push_back(SceneModel("models/viking_room.obj", "textures/simpleColour.png", glm::vec3(-2.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), false));
		//dynamicModels.push_back(SceneModel("models/viking_room.obj", "textures/viking_room.png", 1, glm::vec3(1.0f, -2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), false));
		//dynamicModels.push_back(SceneModel("models/stanford-bunny.obj", "textures/simpleColour.png", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), false));
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

struct TextureInfo {
	std::string fileName;
	std::string fileType;
	bool hasMaps;

	TextureInfo(const std::string& fName, const std::string& fType, bool hasMaps)
		: fileName(fName), fileType(fType), hasMaps(hasMaps) {}
};

struct MaterialInfo {
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;

	MaterialInfo(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) 
		: ambient(ambient), diffuse(diffuse), specular(specular), shininess(shininess) {}
};

class Scene {
public:
	Scene(const char* scenePath);
	
	void cleanup();
	SceneInfo* getSceneInfo();

private:
	SceneInfo* info;
};