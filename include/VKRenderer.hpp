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
			m_window->collect_input();
	}

private:
	void init() {
		const char* title = "TMP";

		m_window = std::unique_ptr<vkc::Window>(new vkc::Window(title, 800, 600));

		m_instance = std::unique_ptr<vkc::Instance>(new vkc::Instance(title, m_window->get_platform_extensions(), {}, {}, {}, {}));
		m_instance->print_info();

		m_surface = m_window->create_surfaceKHR(m_instance->get_handle());

		m_device = std::unique_ptr<vkc::Device>(new vkc::Device(
			m_instance->get_selected_gpu(),
			m_surface,
			{ VK_KHR_SWAPCHAIN_EXTENSION_NAME }
		));

		m_render_context = std::make_unique<vkc::RenderContext>(
			m_instance->get_selected_gpu().get_handle(),
			m_device->get_handle(),
			m_surface,
			m_window->get_current_extent()
		);

	}
private:
	std::unique_ptr<vkc::Instance> m_instance;
	std::unique_ptr<vkc::Window>   m_window;
	std::unique_ptr<vkc::Device>   m_device;

	std::unique_ptr<vkc::RenderContext>  m_render_context;

	std::vector<vkc::RenderPass> m_render_pipeline;

	VkSurfaceKHR m_surface;
};