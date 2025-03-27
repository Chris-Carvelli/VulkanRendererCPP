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
	~VKRenderer() {
		// hack to force swapchain destructor to run before the destruction of the surface
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

private:
	std::unique_ptr<vkc::Instance> m_instance;
	std::unique_ptr<vkc::Window>   m_window;
	std::unique_ptr<vkc::Device>   m_device;
	std::unique_ptr<vkc::RenderContext>  m_render_context;

	VkSurfaceKHR m_surface;
};