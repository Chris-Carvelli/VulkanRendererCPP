#pragma once

#include <vulkan/vulkan.h>


#include <VulkanUtils.h>
#include <core/Instance.hpp>
#include <core/Device.hpp>
#include <core/RenderContext.hpp>
#include <core/RenderPass.hpp>
#include <window/Window.hpp>

#include <memory>

class VKRenderer {
public:
	VKRenderer()
		: m_surface{ 0 }
	{
	};

	void run()
	{
		init();

		while (!m_window->should_quit())
		{
            render();
			m_window->collect_input();
		}
	}

private:
    void init();
    void render();

private:
	std::unique_ptr<vkc::Instance> m_instance;
	std::unique_ptr<vkc::Window>   m_window;
	std::unique_ptr<vkc::Device>   m_device;
	std::unique_ptr<vkc::RenderContext>  m_render_context;
	std::vector<vkc::RenderPass> m_render_passes;

	VkSurfaceKHR m_surface;
};