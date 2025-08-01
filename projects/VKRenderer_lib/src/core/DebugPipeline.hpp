#pragma once

#include <vulkan/vulkan.h>
#include <core/VertexData.h>
#include <vector>


namespace vkc {
	class RenderContext;
	class RenderPass;

	struct DebugPipelineConfig {
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	};

	class DebugPipeline {
	public:
		DebugPipeline(
			VkDevice handle_device,
			vkc::RenderContext* obj_render_context,
			vkc::RenderPass* obj_render_pass,
			DebugPipelineConfig* config
		);
		~DebugPipeline();

		VkPipeline get_handle() const { return m_handle; };
		VkPipelineLayout get_layout() const{ return m_handle_pipeline_layout; };

		void update_uniform_buffer(void* ubo, uint32_t current_frame);
		void bind_descriptor_sets(VkCommandBuffer command_buffer, uint32_t image_index);

	private:
		void create_descriptor_set_layout();
		void create_descriptor_sets();
		void create_uniform_buffers();

		VkShaderModule create_shader_module(
			VkDevice device,
			const char* code,
			const size_t codeSize
		);

	private:
		// back references
		VkDevice m_handle_device;
		vkc::RenderPass* m_obj_render_pass;
		vkc::RenderContext* m_obj_render_context;

		// owned references
		VkPipelineLayout m_handle_pipeline_layout;
		VkPipeline m_handle;
		VkDescriptorSetLayout m_handle_descriptor_set_layout;

		// config
		DebugPipelineConfig* m_config;

		VkDescriptorPool m_descriptor_pool;

		// one of those per frame-in-flight
		std::vector<VkDescriptorSet>	m_descriptor_sets;

		// frame data
		std::vector<VkBuffer>			m_uniform_buffers;
		std::vector<VkDeviceMemory>		m_uniform_buffers_memory;
		std::vector<void*>				m_uniform_buffers_mapped;
	};
}