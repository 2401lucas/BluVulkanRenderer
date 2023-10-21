﻿#include "BluRendererVulkan.h"
#include <chrono>

int main(int argc, char** argv) {
	std::unique_ptr<BluRendererVulkan> blu = std::make_unique<BluRendererVulkan>();
	blu->run(argc, argv);
	return 0;
}

int BluRendererVulkan::run(int argc, char** argv)
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Renderer Example";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Blu Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	DeviceSettings deviceSettings{};
	deviceSettings.enabledDeviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceSettings.msaaSamples = VK_SAMPLE_COUNT_8_BIT;

	windowManager = std::make_unique<WindowManager>(appInfo.pApplicationName);
	renderManager = std::make_unique<RenderManager>(windowManager->getWindow(), appInfo, deviceSettings);
	engineCore = std::make_unique<EngineCore>();

	bool isRunning = true;
	std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	//Gameloop
	while (isRunning)
	{
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		startTime = std::chrono::high_resolution_clock::now();
		//Handles Events & Input
		windowManager->handleEvents();
		// Process Game entities & objects (Send input)
		engineCore->update(time, windowManager->getInput());
		//Potentially reduce rate of update for physics
		engineCore->updatePhysics(time);

		renderManager->drawFrame();
		currentTime = std::chrono::high_resolution_clock::now();
	}

	renderManager.reset();
	windowManager.reset();

	return 0;
}