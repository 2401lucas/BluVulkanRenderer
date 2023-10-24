#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class Camera{
public:
    Camera(const glm::vec3& pos, const glm::vec3& rotation, const float FOV,  const float ratio, const float zNear, const float zFar);

    glm::mat4& getProjMat();
    glm::mat4& getViewMat();
    //TODO: MODIFY POS & ROT
private:
    glm::vec3 position;
    glm::vec3 rotation;

    glm::mat4 view;
    glm::mat4 proj;

    float fov;
    float ratio;
    float zNear;
    float zFar;
};