#include "BluRendererVulkan.h"
#include <chrono>
#include <thread>

const float MINFRAMETIME = 0.01666f;

int main(int argc, char** argv) {
	std::unique_ptr<BluRendererVulkan> blu = std::make_unique<BluRendererVulkan>();
	blu->run(argc, argv);
	return 0;
}

int BluRendererVulkan::run(int argc, char** argv)
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Blu Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
	appInfo.pEngineName = "Blu Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	//TODO, Settings class to handle changes during runtime
	DeviceSettings deviceSettings{};
	deviceSettings.enabledDeviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceSettings.enabledDeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	deviceSettings.enabledDeviceFeatures12.runtimeDescriptorArray = VK_TRUE;
	deviceSettings.enabledDeviceFeatures12.descriptorIndexing = VK_TRUE;
	deviceSettings.enabledDeviceFeatures12.pNext = &deviceSettings.enabledFragShaderBarycentricFeatures;
	deviceSettings.enabledFragShaderBarycentricFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR; 
	deviceSettings.enabledFragShaderBarycentricFeatures.fragmentShaderBarycentric = VK_TRUE;
	deviceSettings.aaSamples = VK_SAMPLE_COUNT_8_BIT;

	windowManager = std::make_unique<WindowManager>(appInfo.pApplicationName);
	engineCore = std::make_unique<EngineCore>(windowManager->getWindow(), appInfo, deviceSettings /*TODO: buildInfo(Scenes to load... ect)*/);
	
	bool isRunning = true;
	std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	//Gameloop
	while (isRunning)
	{
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		startTime = std::chrono::high_resolution_clock::now();
		//Handles Events & Input
		windowManager->handleEvents();
		// Process Game entities & objects (Send input)
		engineCore->update(frameTime, windowManager->getInput(), windowManager->isFramebufferResized());
		//Potentially reduce rate of update for physics
		engineCore->fixedUpdate(frameTime);
		currentTime = std::chrono::high_resolution_clock::now();
	}

	engineCore.reset();
	windowManager.reset();

	return 0;
}