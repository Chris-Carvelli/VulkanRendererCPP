#include <VKRenderer.hpp>

#include <imgui.h>

#include <chrono>

void VKRenderer::run() {
	init_base();
	init();

	while (!m_window->should_quit())
	{
		auto time_start = std::chrono::high_resolution_clock::now();

		vkc::utils::DearImGui::TMP_SingletonInstance->BeginFrame();
		m_render_context->render_begin();

		render();
		gui_record();
		m_render_context->render_finalize();
		auto time_end = std::chrono::high_resolution_clock::now();
		auto x = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start);

		m_app_stats.fps[m_app_stats.curr_frame % AppStats::FPS_SMOOTH_WINDOW_SIZE] = 1000.0f / x.count();
		m_app_stats.curr_frame++;

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


void VKRenderer::gui_record() {
	float smoothed_fps = 0;
	for (int i = 0; i < AppStats::FPS_SMOOTH_WINDOW_SIZE; ++i)
		smoothed_fps += m_app_stats.fps[i];
	ImGui::Begin("App Info");
	ImGui::LabelText("FPS", "%3.0f", smoothed_fps / AppStats::FPS_SMOOTH_WINDOW_SIZE);
	ImGui::End();
}