#include "BluRendererVulkan.h"

#include "Core/Engine/Engine.h"
#include "Core/External/Window.h"
#include "Core/Rendering/ForwardRenderer.h"

const float MINFRAMETIME = 0.01666f;

int main(int argc, char** argv) {
  BluRendererVulkan* blu = new BluRendererVulkan();
  blu->run(argc, argv);

  delete blu;
  return 0;
}

void BluRendererVulkan::run(int argc, char** argv) {
  blu::core::Window* window =
      new blu::core::Window(800, 600, "Blu: Rendering Prototype");
  blu::core::Engine* engine = new blu::core::Engine(window);
  ForwardRenderer* renderer = new ForwardRenderer(window);

  // Engine Loads Initial Scene
  renderer->Prepare();
  engine->LoadScene("TODO", renderer);

  while (!window->ShouldClose()) {
    window->ProcessEvents();
    engine->Update();
    renderer->Render(engine->GetRenderData());
    // - Audio Updates?
  }

  delete renderer;
  delete engine;
  delete window;
}