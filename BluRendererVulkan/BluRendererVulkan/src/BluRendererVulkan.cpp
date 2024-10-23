#include "BluRendererVulkan.h"

// #include "Render/Renderer/ForwardRenderer.hpp"

const float MINFRAMETIME = 0.01666f;

int main(int argc, char** argv) {
  BluRendererVulkan* blu = new BluRendererVulkan();
  blu->run(argc, argv);

  delete blu;
  return 0;
}

void BluRendererVulkan::run(int argc, char** argv) {
  /*BaseRenderer* forwardRenderer = new ForwardRenderer();
  forwardRenderer->start();
  delete (forwardRenderer);*/
}