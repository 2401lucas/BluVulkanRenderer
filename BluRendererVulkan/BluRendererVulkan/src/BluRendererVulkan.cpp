﻿#include "BluRendererVulkan.h"
#include <chrono>
#include <thread>

const float MINFRAMETIME = 0.01666f;

int main(int argc, char** argv) {
	std::unique_ptr<BluRendererVulkan> blu = std::make_unique<BluRendererVulkan>();
	blu->run(argc, argv);
	return 0;
}
//Load build Dependancies
//Create Managers
//Load Scene
//Begin Updates
int BluRendererVulkan::run(int argc, char** argv)
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Blu Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
	appInfo.pEngineName = "Blu Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	DeviceSettings deviceSettings{};
	deviceSettings.enabledDeviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceSettings.enabledDeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	deviceSettings.enabledDeviceFeatures12.runtimeDescriptorArray = VK_TRUE;
	deviceSettings.enabledDeviceFeatures12.descriptorIndexing = VK_TRUE;
	deviceSettings.enabledDeviceFeatures12.pNext = &deviceSettings.enabledFragShaderBarycentricFeatures;
	deviceSettings.enabledFragShaderBarycentricFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR; 
	deviceSettings.enabledFragShaderBarycentricFeatures.fragmentShaderBarycentric = VK_TRUE;
	deviceSettings.msaaSamples = VK_SAMPLE_COUNT_8_BIT;
	//Prepares window & input handling
	windowManager = std::make_unique<WindowManager>(appInfo.pApplicationName);
	//Initializes the Persistant Renderer Components
	renderManager = std::make_unique<RenderManager>(windowManager->getWindow(), appInfo, deviceSettings);
	//Initializes the EngineComponenets
	engineCore = std::make_unique<EngineCore>();

	//Preloads all scene dependencies for rendering manager
	//TODO: Preload only 1 scene at a time
	Scene scene = Scene("temp");

	bool isRunning = true;
	std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	//Gameloop
	while (isRunning)
	{
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		startTime = std::chrono::high_resolution_clock::now();
		//Handle Scene Change (EngineCore Scene setup + recreate Renderer in RenderManager)

		//Handles Events & Input
		windowManager->handleEvents();
		// Process Game entities & objects (Send input)
		engineCore->update(frameTime, windowManager->getInput());
		//Potentially reduce rate of update for physics
		engineCore->updatePhysics(frameTime);

		renderManager->drawFrame(windowManager->isFramebufferResized(), scene.getSceneInfo());
		currentTime = std::chrono::high_resolution_clock::now();
	}

	renderManager.reset();
	windowManager.reset();

	return 0;
}