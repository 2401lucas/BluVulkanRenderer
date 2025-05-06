#include "BluRendererVulkan.h"

#include <chrono>
#include <iostream>

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
      new blu::core::Window(1920, 1080, "Blu: Rendering Prototype");
  ForwardRenderer* renderer = new ForwardRenderer(window);

  blu::core::Engine* engine = new blu::core::Engine(window, renderer);
  engine->LoadScene("TODO");

  auto start_time = std::chrono::high_resolution_clock::now();
  auto current_time = std::chrono::high_resolution_clock::now();
  constexpr double FIXED_TICK_INTERVAL = 1.0 / 60.0;
  double fixed_update_timer = 0.0;

  while (!window->ShouldClose()) {
    current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> frame_duration = current_time - start_time;
    start_time = std::chrono::high_resolution_clock::now();

    window->ProcessEvents();
    engine->Update(frame_duration.count());

    fixed_update_timer += frame_duration.count();
    while (fixed_update_timer >= FIXED_TICK_INTERVAL) {
      engine->FixedUpdate(FIXED_TICK_INTERVAL);
      fixed_update_timer -= FIXED_TICK_INTERVAL;
    }

    renderer->Render(engine->GetRenderData());
  }

  delete renderer;
  delete engine;
  delete window;
}