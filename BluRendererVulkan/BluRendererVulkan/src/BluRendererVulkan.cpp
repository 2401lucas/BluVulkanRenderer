#include "BluRendererVulkan.h"

#include <chrono>
#include <thread>
#include "Render/Renderer/imguiRenderer.hpp"

const float MINFRAMETIME = 0.01666f;

int main(int argc, char** argv) {
  std::unique_ptr<BluRendererVulkan> blu =
      std::make_unique<BluRendererVulkan>();
  blu->run(argc, argv);
  return 0;
}

int BluRendererVulkan::run(int argc, char** argv) {
  std::vector<const char*> args;
  BaseRenderer* vulkanExample = new VulkanExample(args);
  vulkanExample->initVulkan();
  vulkanExample->setupWindow();
  vulkanExample->prepare();
  vulkanExample->renderLoop();
  delete (vulkanExample);	

  return 0;
}