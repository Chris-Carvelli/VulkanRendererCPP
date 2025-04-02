#include <VKRenderer.hpp>

void VKRenderer::run() {
	init_base();
	init();

	while (!m_window->should_quit())
	{
		m_render_context->render_begin();
		render();
		m_window->collect_input();
	}

	// wait for queues to be done before cleanup
	vkDeviceWaitIdle(m_device->get_handle());
}

void VKRenderer::init_base()
{
	const char* title = "TMP";

	m_window = std::unique_ptr<vkc::Window>(new vkc::Window(title, 800, 600));

	m_instance = std::unique_ptr<vkc::Instance>(new vkc::Instance(title, m_window->get_platform_extensions(), {}, {}, {}, {}));
	m_instance->print_info();

	m_surface = m_window->create_surfaceKHR(m_instance->get_handle());

	// this needs to happen, otherwise we can't use PhysicalDevice to query QueueFamilies
	m_instance->set_surface(m_surface);

	m_device = std::unique_ptr<vkc::Device>(new vkc::Device(
		m_instance->get_selected_gpu(),
		m_surface,
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME }
	));

	m_render_context = std::make_unique<vkc::RenderContext>(
		&m_instance->get_selected_gpu(),
		m_device->get_handle(),
		m_surface,
		m_window.get()
	);

	m_dear_imgui = std::make_unique<vkc::utils::DearImGui>();
	m_dear_imgui->Initialize(
		m_window.get(),
		m_instance->get_handle(),
		m_instance->get_selected_gpu().get_handle(),
		m_device->get_handle(),
		// TODO after making pipeline/renderpass creation API,
		//      create UI renderpass and pass it here
		//      (may need also the pipeline, for the frame recording finalization)
		m_render_context->get_renderpass(0)->get_handle()
	);
}