#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class RenderContext;
	class RenderPass;
	class Pipeline;
	class PipelineConfig;

	/// <summary>
	/// Wrapper around descriptors (pool, sets) and per-instance resources
	///		- material uniform buffer resources
	///		- texture samplers
	/// </summary>
	class PipelineInstance {
	public:
		PipelineInstance(
			VkDevice handle_device,
			vkc::RenderContext* obj_render_context,
			vkc::RenderPass* obj_render_pass,
			vkc::Pipeline* obj_pipeline,
			std::vector<VkImageView> image_views
		);

		~PipelineInstance();

		void create_descriptor_sets();
		void update_descriptor_sets();
		void destroy_descriptor_sets();

		void bind_descriptor_sets(VkCommandBuffer command_buffer, uint32_t image_index);

		void update_uniform_buffer_material(void* ubo, uint32_t current_frame);
	private:
		void create_material_uniform_buffers();
		void create_texture_samplers();

		// back references
		VkDevice m_handle_device;
		vkc::RenderPass* m_obj_render_pass;
		vkc::RenderContext* m_obj_render_context;
		vkc::Pipeline* m_obj_pipeline;

		// config
		const PipelineConfig* m_config;

		VkDescriptorPool m_descriptor_pool;
		std::vector<VkDescriptorSet> m_descriptor_sets;

		uint32_t m_pool_size_count;

		// material data
		std::vector<VkBuffer>			m_uniform_buffers_material;
		std::vector<VkDeviceMemory>		m_uniform_buffers_memory_material;
		std::vector<void*>				m_uniform_buffers_mapped_material;
		std::vector<VkSampler>			m_texture_samplers;

		// texture data
		std::vector<VkImageView> m_image_views;

		// aux
		// first texture binding slot
		uint32_t m_first_texture_binding_slot;
	};
}