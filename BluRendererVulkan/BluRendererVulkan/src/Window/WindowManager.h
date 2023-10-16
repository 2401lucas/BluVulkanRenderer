#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core {
	namespace System {
		class WindowManager {
		public:
			WindowManager(VkInstance, const char*);
			~WindowManager();

			void HandleEvents();

			int GetWidth();
			int GetHeight();

			void SetFullscreen(bool fullscreen = true);

			GLFWwindow* GetWindow();
			VkSurfaceKHR GetSurface();

			enum class CursorMode {
				Normal,
				Hidden,
				Disabled,
			};

		private:
			void errorCallback(int, const char*);
			GLFWwindow* window;
			VkSurfaceKHR surface;
		};
	}
}