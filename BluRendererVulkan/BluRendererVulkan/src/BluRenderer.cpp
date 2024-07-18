#include "BluRenderer.h"

#include <chrono>
#include <thread>

const float MINFRAMETIME = 0.01666f;

int main(int argc, char** argv) {
  std::unique_ptr<BluRenderer> blu = std::make_unique<BluRenderer>();
  blu->run(argc, argv);
  return 0;
}

int BluRenderer::run(int argc, char** argv) {
  /*BaseRenderer* forwardRenderer = new ForwardRenderer();
  forwardRenderer->start();
  delete (forwardRenderer);	*/

  return 0;
}