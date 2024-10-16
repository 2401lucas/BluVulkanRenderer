﻿#include "BluRendererVulkan.h"

#include <chrono>
#include <thread>
#include "Render/Renderer/ForwardRenderer.hpp"

const float MINFRAMETIME = 0.01666f;

int main(int argc, char** argv) {
  std::unique_ptr<BluRendererVulkan> blu =
      std::make_unique<BluRendererVulkan>();
  blu->run(argc, argv);
  return 0;
}

int BluRendererVulkan::run(int argc, char** argv) {
  BaseRenderer* forwardRenderer = new ForwardRenderer();
  forwardRenderer->start();
  delete (forwardRenderer);	

  return 0;
}