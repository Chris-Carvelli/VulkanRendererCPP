#pragma once

#include <vulkan/vulkan.h>


#include <VulkanUtils.h>
#include <core/Instance.hpp>
#include <core/Device.hpp>
#include <core/RenderContext.hpp>
#include <core/RenderPass.hpp>
#include <window/Window.hpp>
#include <utils/DearImGui.hpp>

#include <memory>

class VKRenderer {
	struct AppStats {
		static const int FPS_SMOOTH_WINDOW_SIZE = 8;

		float fps[FPS_SMOOTH_WINDOW_SIZE] = { 0 };
		int curr_frame = 0;
	};

public:
	VKRenderer()
		: m_surface{ 0 }
	{
	};
	~VKRenderer() {
		// hack to force swapchain destructor to run before the destruction of the surface
		m_dear_imgui.reset();
		m_render_context.reset();
		m_device.reset();

		vkDestroySurfaceKHR(m_instance->get_handle(), m_surface, NULL);

		m_instance.reset();
		m_window.reset();
	}

	void run();

protected:
	// these could probably be pure virtual
	virtual void init()   { }
	virtual void render() { }

private:
	void init_base();

	void gui_record();
private:
	std::unique_ptr<vkc::Instance> m_instance;
	std::unique_ptr<vkc::Window>   m_window;
	std::unique_ptr<vkc::Device>   m_device;
	std::unique_ptr<vkc::RenderContext>  m_render_context;

	std::unique_ptr<vkc::utils::DearImGui> m_dear_imgui;

	VkSurfaceKHR m_surface;

	AppStats m_app_stats;
};