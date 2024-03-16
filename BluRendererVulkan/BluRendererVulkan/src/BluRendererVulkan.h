#include <memory>
#include "Render/Window/WindowManager.h"
#include "Engine/EngineCore/EngineCore.h"

class BluRendererVulkan {
public:
	int run(int argc, char** argv);

private:
	std::unique_ptr<WindowManager> windowManager;
	std::unique_ptr<EngineCore> engineCore;
}; 