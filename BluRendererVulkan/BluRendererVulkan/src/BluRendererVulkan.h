#include <memory>
#include "Window/WindowManager.h"
#include "Renderer/RenderManager.h"

class BluRendererVulkan {
public:
	int run(int argc, char** argv);

private:
	std::unique_ptr<WindowManager> windowManager;
	std::unique_ptr<RenderManager> renderManager;
};