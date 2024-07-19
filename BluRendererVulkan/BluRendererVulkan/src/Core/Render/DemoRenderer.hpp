#pragma once

#define DEBUG_ALL
#define DEBUG_RENDERGRAPH
#define DEBUG_RENDERER
#define DEBUG_ENGINE

#include "BaseRenderer.h"
#include "Components/Camera.hpp"
#include "Components/Light.hpp"

class DemoRenderer : public BaseRenderer {
 public:
  DemoRenderer() : BaseRenderer() {}

  void getEnabledFeatures() override {}
  void getEnabledExtensions() override {}

  void windowResized() override {}

  void render() override {}
};