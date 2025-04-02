#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class Window {
	public:
		Window(const char* title, int width, int height);
		bool should_quit() const { return m_should_quit; };
		void collect_input();
		std::vector<const char*> get_platform_extensions();
		VkSurfaceKHR create_surfaceKHR(VkInstance instance);
		VkExtent2D get_current_extent();

		// returns appropriate handle depending on the current implementation
		// examples (checked if implemented)
		// - [x] win32 HWND
		// - [ ] SLD3_Window
		// - [ ] etc
		//
		// This voi* is very hacky and unsafe. We Could add an implementation-specific header that
		// exposes a function that returns the correct type, or typedef a `NativeWindowHandle` from the correct type
		// and static_cast it
		void* get_native_window_handle();
	private:
		bool m_should_quit = false;
	};
}