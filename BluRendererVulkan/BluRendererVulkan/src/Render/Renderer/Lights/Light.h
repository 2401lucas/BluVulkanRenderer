#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace vks {
namespace light {
const char* debugLightType[3] = {"Directional", "Area", "Spot"};

struct GPULightInfo {
  glm::vec4 color;
  glm::vec4 position;
  glm::vec4 rotation;
  glm::vec4 lightFalloff;
};

struct Light {
  glm::vec4 color;  // XYZ - RGB, W - Intensity
  glm::vec3 position;
  glm::vec3 rotation;
  // Keep depth range as small as possible for better shadow map precision
  int lightType;
  float lightFOV;
  float lightConst;
  float lightLinear;
  float lightQuadratic;

  float aspect;
  float zNear;
  float zFar;

  glm::mat4 depthProjectionMatrix;
  glm::mat4 depthViewMatrix;
  glm::mat4 lightSpace;

  GPULightInfo lightInfo;

  void updateLight() {
    lightInfo = GPULightInfo();
    lightInfo.color = glm::vec4(glm::vec3(color) * color.w, lightType);

    // Directional Light
    if (lightType == 0) {
      depthProjectionMatrix =
          glm::perspective(glm::radians(lightFOV), aspect, zNear, zFar);
      depthViewMatrix = glm::lookAt(glm::vec3(rotation * -10.0f), position,
                                    glm::vec3(0, 1, 0));

      lightSpace = depthProjectionMatrix * depthViewMatrix;
      // Light info for packed for GPU
      lightInfo.rotation = glm::vec4(rotation, 0.0f);
    }
    // Point Light
    else if (lightType == 1) {
      lightInfo.position = glm::vec4(position, 0.0f);
      lightInfo.lightFalloff =
          glm::vec4(lightConst, lightLinear, lightQuadratic, 0.0f);
    }
    // Spot Light
    else if (lightType == 2) {
      lightInfo.position = glm::vec4(position, glm::radians(lightFOV));
      lightInfo.rotation = glm::vec4(rotation, 0.0f);
      lightInfo.lightFalloff =
          glm::vec4(lightConst, lightLinear, lightQuadratic, 0.0f);
    }
  }

  // Directional Light: TODO, figure out shadows
  void createDirectionalLight(glm::vec4 col, glm::vec3 dir,
                              glm::vec3 target, float fov,
                              float aspect, float zNear, float zFar) {
    lightType = 0;
    color = col;
    rotation = dir;
    position = target;
    lightFOV = fov;
    this->aspect = aspect;
    this->zNear = zNear;
    this->zFar = zFar;
    updateLight();
  }

  // Point Light
  void createPointLight(glm::vec4 col, glm::vec3 pos, float constant,
                        float linear, float quadratic) {
    lightType = 1;
    color = col;
    position = pos;
    lightConst = constant;
    lightLinear = linear;
    lightQuadratic = quadratic;
    updateLight();
  }

  // Spot Light
  void createSpotLight(glm::vec4 col, glm::vec3 pos, glm::vec3 dir, float fov,
                       float constant, float linear, float quadratic) {
    lightType = 2;
    color = col;
    position = pos;
    rotation = dir;
    lightFOV = fov;
    lightConst = constant;
    lightLinear = linear;
    lightQuadratic = quadratic;
    updateLight();
  }
};
}  // namespace light
}  // namespace vks