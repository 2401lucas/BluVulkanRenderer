#pragma once

#include "WindowManager.h"
#include "RenderManager.h"
#include <iostream>

namespace Core {
	class BluRendererVulkan {
	public:
		int run(int, char**);

	private:
		std::unique_ptr<Core::System::WindowManager> windowManager;
		std::unique_ptr<Core::Rendering::RenderManager> renderManager;
	};
}