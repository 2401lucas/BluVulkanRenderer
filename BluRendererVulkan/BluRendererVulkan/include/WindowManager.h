#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core {
	namespace System {
		class WindowManager {
		public:
			WindowManager();
			~WindowManager();

			void HandleEvents();

			int GetWidth();
			int GetHeight();

			void SetFullscreen(bool fullscreen = true);

			GLFWwindow* GetWindow();

			enum class CursorMode {
				Normal,
				Hidden,
				Disabled,
			};

		private:
			GLFWwindow* window;
		};
	}
}