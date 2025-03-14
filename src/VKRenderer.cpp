#include <VKRenderer.hpp>

void VKRenderer::init()
{
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

void VKRenderer::render()
{

}