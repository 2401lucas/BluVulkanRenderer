#include "BluRendererVulkan.h"

#include <chrono>

#include "Core/Engine/Engine.h"
#include "Core/External/Window.h"
#include "Core/Rendering/ForwardRenderer.h"
#include <iostream>

const float MINFRAMETIME = 0.01666f;

int main(int argc, char** argv) {
  BluRendererVulkan* blu = new BluRendererVulkan();
  blu->run(argc, argv);

  delete blu;
  return 0;
}

void BluRendererVulkan::run(int argc, char** argv) {
  blu::core::Window* window =
      new blu::core::Window(1920, 1080, "Blu: Rendering Prototype");
  ForwardRenderer* renderer = new ForwardRenderer(window);

  blu::core::Engine* engine = new blu::core::Engine(window, renderer);
  engine->LoadScene("TODO");

  auto start_time = std::chrono::high_resolution_clock::now();
  auto current_time = std::chrono::high_resolution_clock::now();

  while (!window->ShouldClose()) {
    current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> frame_duration = current_time - start_time;
    start_time = std::chrono::high_resolution_clock::now();

    window->ProcessEvents();
    engine->Update(frame_duration.count());
    renderer->Render(engine->GetRenderData());
    /*if (res == RendererState::ASPECT_RATIO_UPDATED) {
      engine->SetCameraAspectRatio(renderer->GetAspectRatio());
    }*/
  }

  delete renderer;
  delete engine;
  delete window;
}