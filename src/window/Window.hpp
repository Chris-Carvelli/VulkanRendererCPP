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

	private:
		bool m_should_quit = false;
	};
}