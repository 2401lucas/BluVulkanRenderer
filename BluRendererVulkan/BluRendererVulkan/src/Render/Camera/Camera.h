#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../Engine/Scene/Scene.h"

class Camera{
public:
    Camera(const glm::vec3& pos, const glm::vec3& rotation, const float FOV,  const float ratio, const float zNear, const float zFar);

    void updateCamera(const SceneCamera& cameraInfo);
    glm::mat4& getProjMat();
    glm::mat4& getViewMat();
private:
    void updateViewMatrix();

    glm::vec3 position;
    glm::vec3 rotation;

    glm::mat4 view;
    glm::mat4 proj;

    float fov;
    float ratio;
    float zNear;
    float zFar;
};