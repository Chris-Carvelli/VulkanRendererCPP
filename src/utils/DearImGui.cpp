#include "DearImGui.hpp"

#include <window/Window.hpp>

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

namespace vkc {
	namespace utils {
        DearImGui* DearImGui::TMP_SingletonInstance = nullptr;

        DearImGui::DearImGui()
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
        }

        DearImGui::~DearImGui()
        {
            Cleanup();
            ImGui::DestroyContext();
        }

        void DearImGui::Initialize(
            vkc::Window* p_window,
            VkInstance g_Instance,
            VkPhysicalDevice g_PhysicalDevice,
            VkDevice g_Device,
            VkRenderPass g_RenderPass
        ) {


            TMP_SingletonInstance = this;

            m_device = g_Device;

            // queue family: let ImGui figure it out. We can force it later
            uint32_t g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);

            // queue: let ImGui figure it out. We can force it later
            VkQueue g_Queue;
            vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);

            // descriptor pool
            // from https://github.com/ocornut/imgui/blob/master/examples/example_win32_vulkan/main.cpp
            // If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
            {
                VkDescriptorPoolSize pool_sizes[] =
                {
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
                };
                VkDescriptorPoolCreateInfo pool_info = {};
                pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                pool_info.maxSets = 0;
                for (VkDescriptorPoolSize& pool_size : pool_sizes)
                    pool_info.maxSets += pool_size.descriptorCount;
                pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
                pool_info.pPoolSizes = pool_sizes;
                vkCreateDescriptorPool(g_Device, &pool_info, nullptr, &m_DescriptorPool);
            }

            // TODO make implementati
            ImGui_ImplWin32_Init(p_window->get_native_window_handle());
            ImGui_ImplVulkan_InitInfo init_info = {};
            //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
            init_info.Instance = g_Instance;
            init_info.PhysicalDevice = g_PhysicalDevice;
            init_info.Device = g_Device;
            init_info.QueueFamily = g_QueueFamily;
            init_info.Queue = g_Queue;
            init_info.PipelineCache = nullptr;
            init_info.DescriptorPool = m_DescriptorPool;
            init_info.RenderPass = g_RenderPass;
            init_info.Subpass = 0;
            init_info.MinImageCount = MIN_IMAGE_COUNT;
            // TODO figure out how this is computed (seems like it dependes on how many textures you want to render in ImGUI?
            init_info.ImageCount = MIN_IMAGE_COUNT;
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            init_info.Allocator = nullptr;
            // TODO this may be needed, but otherwise we have our own debug messaging
            init_info.CheckVkResultFn = nullptr; 
            ImGui_ImplVulkan_Init(&init_info);
        }

        void DearImGui::Cleanup()
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplWin32_Shutdown();

            vkDestroyDescriptorPool(m_device, m_DescriptorPool, nullptr);
        }

        void DearImGui::BeginFrame()
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        void DearImGui::EndFrame(VkCommandBuffer cmd_buffer)
        {
            ImGui::Render();
            // TODO check if RenderDrawData's third parameter (VkPipeline, default to nullptr) is needed
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd_buffer);
        }
	}
}