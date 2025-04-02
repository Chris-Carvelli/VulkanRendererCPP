#pragma once

#include <vulkan/vulkan.h>

namespace vkc {
    class Window;

	namespace utils {
		class DearImGui {
            const int MIN_IMAGE_COUNT = 2;

        public:
            DearImGui();
            ~DearImGui();

            void Initialize(
                vkc::Window* p_window,
                VkInstance g_Instance,
                VkPhysicalDevice g_PhysicalDevice,
                VkDevice g_Device,
                VkRenderPass g_RenderPass
            );

            void Cleanup();

            void BeginFrame();
            void EndFrame(VkCommandBuffer cmd_buffer);
		};
	}
}