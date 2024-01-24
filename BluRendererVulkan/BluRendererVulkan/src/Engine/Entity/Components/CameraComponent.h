#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include "BaseComponent.h"

struct Camera : BaseComponent {
public:
    glm::mat4 view;
    glm::mat4 proj;

    float fov;
    float ratio;
    float zNear;
    float zFar;
};