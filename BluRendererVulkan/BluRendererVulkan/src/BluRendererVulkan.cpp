#include "../include/BluRendererVulkan.h"

int main(int argc, char** argv) {
	std::unique_ptr<Core::BluRendererVulkan> blu = std::make_unique <Core::BluRendererVulkan>();
	blu->run(argc, argv);
	return 0;
}

int Core::BluRendererVulkan::run(int argc, char** argv)
{
	windowManager = std::make_unique<Core::System::WindowManager>();
	renderManager = std::make_unique<Core::Rendering::RenderManager>(windowManager->GetWindow());

	bool isRunning = true;

	//Gameloop
	while (isRunning)
	{
		//Handles Events & Input
		windowManager->HandleEvents();
		// Process Game entities & objects (Send input)
		// Update Physics
		renderManager->renderFrame();
	}

	renderManager.reset();
	windowManager.reset();

	return 0;
}