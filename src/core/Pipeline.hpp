#pragma once

#include <vulkan/vulkan.h>
#include <core/VertexData.h>
#include <vector>


namespace vkc {
	class RenderContext;

	class Pipeline {
	protected:
		Pipeline()
			: m_handle_device{ VK_NULL_HANDLE }
			, m_handle{ VK_NULL_HANDLE }
			, m_handle_render_pass{ VK_NULL_HANDLE }
			, m_obj_render_context{ nullptr }
		{ };

	public:
		Pipeline(
			VkDevice handle_device,
			vkc::RenderContext* obj_render_context,
			VkRenderPass handle_render_pass,
			const char* vert_path,
			const char* frag_path,
			VkCullModeFlags face_culling_mode = VK_CULL_MODE_BACK_BIT
		);
		virtual ~Pipeline();

		virtual VkPipeline get_handle() const { return m_handle; };
		virtual VkPipelineLayout get_layout() const{ return m_handle_pipeline_layout; };

		virtual void update_uniform_buffer(DataUniformFrame& ubo, uint32_t current_frame);
		virtual void update_uniform_buffer_material(DataUniformMaterial& ubo, uint32_t current_frame);
		virtual void bind_descriptor_sets(VkCommandBuffer command_buffer, uint32_t image_index);

	private:
		void create_descriptor_set_layout();
		void create_descriptor_sets();
		void create_uniform_buffers();
		void create_texture_samplers();

		VkShaderModule create_shader_module(
			VkDevice device,
			const char* code,
			const size_t codeSize
		);

	private:
		// back references
		VkDevice m_handle_device;
		VkRenderPass m_handle_render_pass;
		vkc::RenderContext* m_obj_render_context;

		// owned references
		VkPipelineLayout m_handle_pipeline_layout;
		VkPipeline m_handle;
		// TODO create automatically from shader
		VkDescriptorSetLayout m_handle_descriptor_set_layout;

		// one of those per frame-in-flight
		VkDescriptorPool m_descriptor_pool;
		std::vector<VkDescriptorSet>	m_descriptor_sets;

		// frame data
		std::vector<VkBuffer>			m_uniform_buffers;
		std::vector<VkDeviceMemory>		m_uniform_buffers_memory;
		std::vector<void*>				m_uniform_buffers_mapped;

		// material data
		std::vector<VkBuffer>			m_uniform_buffers_material;
		std::vector<VkDeviceMemory>		m_uniform_buffers_memory_material;
		std::vector<void*>				m_uniform_buffers_mapped_material;

		// data
		// eventually we want to split image from sampler. It depends on how the VkWriteDescriptorSet is set up
		// see https://docs.vulkan.org/samples/latest/samples/api/separate_image_sampler/README.html
		// what we want is
		// - pipeline: VkSampler only
		// - application
		//    - VkImage
		//    - VkDeviceMemory
		//    - VkImageView
		// this requires different shaders too (separate setup uses two uniforms)
		// Default is probably split, but there are images that makes sense to keep unified
		// - shadow/light maps
		// - noise textures
		// - ...
		VkSampler m_texture_sampler;
	};
}