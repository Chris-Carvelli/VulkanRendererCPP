#include <VKRenderer.hpp>

#include <core/DrawCall.hpp>

#include <imgui.h>

#include <chrono>

void VKRenderer::run() {
	init_base();
	init();

	// force upload of assets to GPU
	TMP_force_gpu_upload_all();
	TMP_create_renderpasses();

	while (!m_window->should_quit())
	{
		auto time_start = std::chrono::high_resolution_clock::now();

		update_base();
		update();

		m_render_context->render_begin();

		render();


		// ideally, begind and end frame would encompass GUI recording only
		// ATM, some GUI is recoded in render_begin(), so we can't yet
		vkc::utils::DearImGui::TMP_SingletonInstance->BeginFrame();
		gui_base();
		gui();
		vkc::utils::DearImGui::TMP_SingletonInstance->EndFrame();

		m_render_context->render_finalize();
		auto time_end = std::chrono::high_resolution_clock::now();
		auto x = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start);

		m_app_stats.fps[m_app_stats.curr_frame % AppStats::FPS_SMOOTH_WINDOW_SIZE] = 1000.0f / x.count();
		m_app_stats.curr_frame++;
		m_app_stats.delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - m_app_stats.curr_time).count() / 1000.0f;
		m_app_stats.curr_time = time_end;


		m_window->collect_input();
	}

	// wait for queues to be done before cleanup
	vkDeviceWaitIdle(m_device->get_handle());
}

void VKRenderer::drawcall_add(
	vkc::Assets::IdAssetMesh id_mesh,
	vkc::Assets::IdAssetMaterial id_material,
	void* uniform_data_model,
	uint32_t uniform_data_model_size
) {
	vkc::Assets::MaterialData& material = vkc::Assets::get_material_data(id_material);

	auto obj_renderpass = m_render_context->get_renderpass(material.id_render_pass);
	auto obj_pipeline = obj_renderpass->get_pipeline_ptr(material.id_pipeline);

	// TODO fix this mess
	// - ATM renderContext cares about renderpasses and pipeline, let it create the drawcall data
	// - idx_data_attributes should be of type `IdAssetMesh`
	// - avoid unecessary casts (we will probably just move them down the call stack, but they make no sense here)
	vkc::Drawcall::add_drawcall(vkc::Drawcall::DrawcallData{
		.obj_render_pass = obj_renderpass,
		.obj_pipeline = obj_pipeline,
		.idx_data_attributes = id_mesh,
		.data_uniform_material = (DataUniformMaterial*)material.uniform_data_material,
		.data_uniform_model = uniform_data_model,
		.data_uniform_model_size = uniform_data_model_size
	});
}

void VKRenderer::TMP_force_gpu_upload_all() {
	for (int i = 0; i < vkc::Assets::get_num_mesh_assets(); ++i)
		vkc::Drawcall::createModelBuffers(i, m_device->get_handle(), m_render_context.get());

	for (int i = 0; i < vkc::Assets::get_num_texture_assets(); ++i)
		vkc::Drawcall::createTextureImage(i, m_device->get_handle(), m_render_context.get());
}

void VKRenderer::TMP_create_renderpasses() {

	// TODO configure renderpass and pipelines
	uint32_t default_pipeline = m_render_context->get_renderpass(0)->add_pipeline(new vkc::PipelineConfig{
				.vert_path = "res/shaders/shader_base.vert.spv",
				.frag_path = "res/shaders/shader_base.frag.spv",
				.size_uniform_data_frame = sizeof(DataUniformFrame),
				.size_uniform_data_material = sizeof(DataUniformMaterial),
				.size_push_constant_model = sizeof(DataUniformModel),
				.vertex_binding_descriptors = vertexData_getBindingDescriptions(),
				.vertex_binding_descriptors_count = vertexData_getBindingDescriptionsCount(),
				.vertex_attribute_descriptors = vertexData_getAttributeDescriptions(),
				.vertex_attribute_descriptors_count = vertexData_getAttributeDescriptionsCount(),
				.texture_image_views = new VkImageView[] {
					vkc::Drawcall::get_texture_image_view(0)
				},
				.texture_image_views_count = 1,
				.face_culling_mode = VK_CULL_MODE_BACK_BIT
		});
	vkc::Instance::TMP_get_singleton_instance()->add_object_debug_name(
		(uint64_t)m_render_context->get_renderpass(0)->get_pipeline_handle(default_pipeline),
		VK_OBJECT_TYPE_PIPELINE,
		m_device->get_handle(),
		"default pipeline"
	);

	uint32_t trail_pipeline = m_render_context->get_renderpass(0)->add_pipeline(new vkc::PipelineConfig{
				.vert_path = "res/shaders/shader_trail.vert.spv",
				.frag_path = "res/shaders/shader_trail.frag.spv",
				.size_uniform_data_frame = sizeof(DataUniformFrame),
				.size_uniform_data_material = sizeof(DataUniformMaterial),
				.size_push_constant_model = sizeof(DataUniformTrail),
				.vertex_binding_descriptors = vertexData_getBindingDescriptions(),
				.vertex_binding_descriptors_count = vertexData_getBindingDescriptionsCount(),
				.vertex_attribute_descriptors = vertexData_getAttributeDescriptions(),
				.vertex_attribute_descriptors_count = vertexData_getAttributeDescriptionsCount(),
				.texture_image_views = new VkImageView[] {
					//vkc::Drawcall::get_texture_image_view(1)
				},
				.texture_image_views_count = 0,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				.face_culling_mode = VK_CULL_MODE_NONE
		});
	vkc::Instance::TMP_get_singleton_instance()->add_object_debug_name(
		(uint64_t)m_render_context->get_renderpass(0)->get_pipeline_handle(trail_pipeline),
		VK_OBJECT_TYPE_PIPELINE,
		m_device->get_handle(),
		"trail pipeline"
	);
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
		m_render_context->get_renderpass(0)->get_handle(),
		m_render_context->get_num_render_frames()
	);

	vkc::Assets::asset_manager_init();

	// init app data
	m_app_stats.curr_frame = 0;
	m_app_stats.curr_time = std::chrono::high_resolution_clock::now();
	m_app_stats.delta_time = 0;
}

void VKRenderer::update_base() {
	auto extents = m_window->get_current_extent();
	m_window_size.width = extents.width;
	m_window_size.height = extents.height;
}

void VKRenderer::gui_base() {
	float smoothed_fps = 0;
	for (int i = 0; i < AppStats::FPS_SMOOTH_WINDOW_SIZE; ++i)
		smoothed_fps += m_app_stats.fps[i];
	ImGui::Begin("App Info");
	ImGui::LabelText("FPS", "%3.0f", smoothed_fps / AppStats::FPS_SMOOTH_WINDOW_SIZE);
	ImGui::LabelText("Delta", "%3.4f", m_app_stats.delta_time);
	ImGui::End();
}