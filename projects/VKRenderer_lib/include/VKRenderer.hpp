#pragma once

#include <vulkan/vulkan.h>

#include <VulkanUtils.h>
#include <AssetManager.hpp>
#include <core/DataStructures.hpp>
#include <core/Instance.hpp>
#include <core/Device.hpp>
#include <core/RenderContext.hpp>
#include <core/RenderPass.hpp>
#include <window/Window.hpp>

extern "C" {
	#include <cc_allocator.h>
	#include <cc_profiler.h>
}

#include <utils/DearImGui.hpp>

#include <chrono>
#include <memory>

class VKRenderer {
	struct AppConfig {
		bool show_debug_ui;
	};

	struct AppStats {
		static const int FPS_SMOOTH_WINDOW_SIZE = 8;

		float fps[FPS_SMOOTH_WINDOW_SIZE] = { 0 };
		int curr_frame = 0;

		std::chrono::steady_clock::time_point curr_time;
		float delta_time;
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
	virtual void update() { }
	virtual void render() { }
	virtual void gui()    { }

	// set state
	void drawcall_add(
		vkc::Assets::IdAssetMesh id_mesh,
		vkc::Assets::IdAssetMaterial id_material,
		void* uniform_data_model,
		uint32_t uniform_data_model_size
	);
	DataUniformFrame& get_ubo_reference() { return m_render_context->get_ubo_reference(); };

	// upload ALL buffers to GPU, without checking if already done
	void TMP_force_gpu_upload_all();

	void TMP_hot_reload();

	// retrieve info
	vkc::Rect2DI get_window_size() const { return m_window_size; };
	int get_current_frame() const { return m_app_stats.curr_frame; };
	float get_delta_time() const { return m_app_stats.delta_time; };

	// TMP this is too low level to handle to the app
	VkDevice get_device_handle() const { return m_device->get_handle(); };
	// TMP this is too low level to handle to the app
	vkc::RenderContext* get_render_context_obj() const { return m_render_context.get(); };
	// TMP this is too low level to handle to the app
	vkc::Instance* get_instance_obj() const { return m_instance.get(); };

private:
	void init_base();
	void update_base();
	void gui_base();
private:
	std::unique_ptr<vkc::Instance> m_instance;
	std::unique_ptr<vkc::Window>   m_window;
	std::unique_ptr<vkc::Device>   m_device;
	std::unique_ptr<vkc::RenderContext>  m_render_context;

	std::unique_ptr<vkc::utils::DearImGui> m_dear_imgui;

	VkSurfaceKHR m_surface;

	AppConfig m_app_config;
	AppStats m_app_stats;

	// internal state
	vkc::Rect2DI m_window_size;

	BumpAllocator* m_allocator;
	Profiler*      m_profiler;
};