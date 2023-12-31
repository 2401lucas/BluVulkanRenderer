#pragma once

#include "../src/Render/Device/Device.h"
#include "../src/Render/Buffer/Buffer.h"
#include "../include/RenderConst.h"
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

struct GPUCameraData {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::mat4 model[RenderConst::MAX_MODELS];
};

struct LightInfo {
	glm::vec4 position;	//XYZ for position, W for light type
	glm::vec4 rotation;	//XYZ  for rotation
	glm::vec4 colour;	//XYZ for RGB, W for Intensity
	glm::vec4 distInfo;	// X for Const, Y for Linear, Z for Quad

	LightInfo() {
		position = glm::vec4(0);
		rotation = glm::vec4(0);
		colour = glm::vec4(0);
		distInfo = glm::vec4(0);
	}
	//Light Types
	//Type 1 - Directional
	//Type 2 - Point
	//Type 3 - Spot
	LightInfo(int lightType, glm::vec3 pos, glm::vec3 rot, glm::vec4 col, float constant, float linear, float quad, float innerCutoff, float outerCutoff) {
		position = glm::vec4(pos, lightType);
		rotation = glm::vec4(rot, glm::radians(innerCutoff));
		colour = col;
		distInfo = glm::vec4(constant, linear, quad, glm::radians(outerCutoff));
	}
};

struct GPUSceneData {
	glm::vec4 cameraPosition;	//XYZ for position, W for number of Lights
	glm::vec4 ambientColor;		//XYZ for RGB, W for Intensity
	LightInfo lightInfo[RenderConst::MAX_LIGHTS];
};

struct Material {
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular; //W is Shininess

	Material() {
		ambient = glm::vec4(0);
		diffuse = glm::vec4(0);
		specular = glm::vec4(0);
	}
};

struct GPUMaterialData {
	Material materials[RenderConst::MAX_MATERIALS];
};

class UBO {
public:
	UBO(Device*, VkDeviceSize, uint32_t);

	Buffer* getBuffer(const uint32_t index);

private:
	std::vector<Buffer*> buffers;
};