#pragma once

#include <vulkan/vulkan.h>

namespace vkc {
    class Window;

	namespace utils {
		class DearImGui {
            const int MIN_IMAGE_COUNT = 2;
        public:
            static DearImGui * TMP_SingletonInstance;

        public:
            DearImGui();
            ~DearImGui();

            void Initialize(
                vkc::Window* p_window,
                VkInstance g_Instance,
                VkPhysicalDevice g_PhysicalDevice,
                VkDevice g_Device,
                VkRenderPass g_RenderPass,
                uint32_t swapchain_image_count
            );

            void Cleanup();

            void BeginFrame();
            void EndFrame(VkCommandBuffer cmd_buffer);
            
        private:
            VkDevice m_device;
            VkDescriptorPool m_DescriptorPool;
        };
    }
}