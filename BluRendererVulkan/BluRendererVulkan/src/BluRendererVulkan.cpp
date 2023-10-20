#include "BluRendererVulkan.h"

int main(int argc, char** argv) {
	std::unique_ptr<BluRendererVulkan> blu = std::make_unique<BluRendererVulkan>();
	blu->run(argc, argv);
	return 0;
}

int BluRendererVulkan::run(int argc, char** argv)
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	DeviceSettings deviceSettings{};
	deviceSettings.enabledDeviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceSettings.msaaSamples = VK_SAMPLE_COUNT_8_BIT;


	windowManager = std::make_unique<WindowManager>("Blu Renderer");
	renderManager = std::make_unique<RenderManager>(windowManager->GetWindow(), appInfo, deviceSettings);

	bool isRunning = true;

	//Gameloop
	while (isRunning)
	{
		//Handles Events & Input
		windowManager->HandleEvents();
		// Process Game entities & objects (Send input)
		// Update Physics
		renderManager->drawFrame();
	}

	renderManager.reset();
	windowManager.reset();

	return 0;
}