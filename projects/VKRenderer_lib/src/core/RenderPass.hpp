#pragma once

#include <vulkan/vulkan.h>

#include <core/Pipeline.hpp>
#include <core/DebugPipeline.hpp>
#include <core/PipelineInstance.hpp>

#include <vector>
#include <memory>

namespace vkc {
	class RenderContext;

	// sore RenderPass for now
	class RenderPass {
	public:
		RenderPass(VkDevice device, RenderContext* obj_render_context);
		~RenderPass();

		VkRenderPass get_handle() const { return m_handle; };

		VkRenderPassBeginInfo get_being_info(uint8_t frame_index) const;
		uint8_t get_pipelines_count() const;
		VkPipeline get_pipeline_handle(uint8_t i);

		uint32_t get_pipeline_instance_count() const { return m_pipeline_instances.size(); }

		// NOT THREAD SAFE!
		vkc::Pipeline* get_pipeline_ptr(uint8_t i);
		vkc::DebugPipeline* get_debug_pipeline_ptr(uint8_t i);
		vkc::PipelineInstance* get_pipeline_instance_ptr(uint8_t i);

		uint32_t add_pipeline(const PipelineConfig* config);
		uint32_t add_pipeline_instance(
			uint32_t pipeline_config_idx,
			std::vector<VkImageView> image_views
		);

		// TODO framebuffer only cares about swaphacin recreation
		void handle_swapchain_recreation();

		VkImageView get_depth_resource() const { return m_depth_image_view; };
	private:
		void create_framebuffers();
		void create_depth_resources();
		void create_pipelines();

		// TODO framebuffer only cares about swaphacin recreation
		void handle_swapchain_destruction();
	private:
		// back-references
		VkDevice m_handle_device;
		RenderContext* m_obj_render_context;

		// owned references
		VkRenderPass m_handle;
		std::vector<VkFramebuffer> m_handle_framebuffers;
		std::vector<std::unique_ptr<Pipeline>> m_pipelines;
		std::vector<std::unique_ptr<DebugPipeline>> m_debug_pipelines;

		std::vector<std::unique_ptr<PipelineInstance>> m_pipeline_instances;

		VkImage			m_depth_image;
		VkDeviceMemory	m_depth_image_memory;
		VkImageView		m_depth_image_view;

		// one per attachment, in attachment order

		const VkClearValue m_clear_values[2] = {
			(VkClearValue) { .color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 1.0f }} },
			(VkClearValue) { .depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 } }
		};
		const int m_clear_values_count = 2;
	};
}