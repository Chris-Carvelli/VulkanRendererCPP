#pragma once

#include <vulkan/vulkan.h>
#include <core/VertexData.h>
#include <vector>


namespace vkc {
	class RenderContext;
	class RenderPass;

	enum PipelineConfigFlags : uint8_t {
		DYNAMIC = 0b0001,
		MULTI   = 0b0010
	};

	struct PipelineConfig {
		const char* vert_path;
		const char* frag_path;

		const VkVertexInputBindingDescription* vertex_binding_descriptors;
		const VkVertexInputAttributeDescription* vertex_attribute_descriptors;
		VkImageView* texture_image_views;

		const uint32_t size_uniform_data_frame;
		const uint32_t size_uniform_data_material;

		const uint32_t size_push_constant_model;
		const uint32_t vertex_binding_descriptors_count;

		const uint32_t vertex_attribute_descriptors_count;
		uint32_t texture_image_views_count;

		const uint8_t flags;
		// fixed pipeline config
		const VkPrimitiveTopology topology      = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		const VkCullModeFlags face_culling_mode = VK_CULL_MODE_FRONT_BIT;
		const VkCompareOp compare_op            = VK_COMPARE_OP_LESS;
	};

	const uint32_t PIPELINE_CONFIG_ID_SKYBOX = 0;
	const uint32_t PIPELINE_CONFIG_ID_PBR = 1;
	const PipelineConfig PIPELINE_CONFIGS[] = {
		{
			.vert_path = "res/shaders/skybox.vert.spv",
			.frag_path = "res/shaders/skybox.frag.spv",
			.size_uniform_data_frame    = sizeof(DataUniformFrame),
			.size_uniform_data_material = 0,
			.size_push_constant_model   = 0,
			.vertex_binding_descriptors         = vertexData_getBindingDescriptions_Skybox(),
			.vertex_binding_descriptors_count   = vertexData_getBindingDescriptionsCount_Skybox(),
			.vertex_attribute_descriptors       = vertexData_getAttributeDescriptions_Skybox(),
			.vertex_attribute_descriptors_count = vertexData_getAttributeDescriptions_SkyboxCount(),
			.face_culling_mode = VK_CULL_MODE_NONE,
			.compare_op = VK_COMPARE_OP_EQUAL
		},
		{
			.vert_path = "res/shaders/pbr.vert.spv",
			.frag_path = "res/shaders/pbr.frag.spv",
			.size_uniform_data_frame    = sizeof(DataUniformFrame),
			.size_uniform_data_material = sizeof(DataUniformMaterial),
			.size_push_constant_model   = sizeof(DataUniformModel),
			.vertex_binding_descriptors         = vertexData_getBindingDescriptions(),
			.vertex_binding_descriptors_count   = vertexData_getBindingDescriptionsCount(),
			.vertex_attribute_descriptors       = vertexData_getAttributeDescriptions(),
			.vertex_attribute_descriptors_count = vertexData_getAttributeDescriptionsCount(),
			.face_culling_mode = VK_CULL_MODE_BACK_BIT
		},
		{
			.vert_path = "res/shaders/base.vert.spv",
			.frag_path = "res/shaders/unlit.frag.spv",
			.size_uniform_data_frame    = sizeof(DataUniformFrame),
			.size_uniform_data_material = 0,
			.size_push_constant_model   = sizeof(DataUniformModel),
			.vertex_binding_descriptors         = vertexData_getBindingDescriptions_Unlit(),
			.vertex_binding_descriptors_count   = vertexData_getBindingDescriptionsCount_Unlit(),
			.vertex_attribute_descriptors       = vertexData_getAttributeDescriptions_Unlit(),
			.vertex_attribute_descriptors_count = vertexData_getAttributeDescriptions_UnlitCount(),
			.face_culling_mode = VK_CULL_MODE_BACK_BIT
		}
	};

	class Pipeline {
	public:
		Pipeline(
			VkDevice handle_device,
			vkc::RenderContext* obj_render_context,
			vkc::RenderPass* obj_render_pass,
			PipelineConfig* config
		);
		~Pipeline();

		VkPipeline get_handle() const { return m_handle; };
		VkPipelineLayout get_layout() const{ return m_handle_pipeline_layout; };

		void update_uniform_buffer(void* ubo, uint32_t current_frame);
		void update_uniform_buffer_material(void* ubo, uint32_t current_frame);
		void update_material_textures(std::vector<VkImageView> image_views, uint32_t current_frame);
		void bind_descriptor_sets(VkCommandBuffer command_buffer, uint32_t image_index);

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
		vkc::RenderPass* m_obj_render_pass;
		vkc::RenderContext* m_obj_render_context;

		// owned references
		VkPipelineLayout m_handle_pipeline_layout;
		VkPipeline m_handle;
		VkDescriptorSetLayout m_handle_descriptor_set_layout;

		// config
		PipelineConfig* m_config;
		
		VkDescriptorPool m_descriptor_pool;

		// one of those per frame-in-flight
		std::vector<VkDescriptorSet>	m_descriptor_sets;

		// frame data
		std::vector<VkBuffer>			m_uniform_buffers;
		std::vector<VkDeviceMemory>		m_uniform_buffers_memory;
		std::vector<void*>				m_uniform_buffers_mapped;

		// material data
		std::vector<VkBuffer>			m_uniform_buffers_material;
		std::vector<VkDeviceMemory>		m_uniform_buffers_memory_material;
		std::vector<void*>				m_uniform_buffers_mapped_material;
		std::vector<VkSampler>			m_texture_samplers;

		// aux
		// first texture binding slot
		uint32_t m_first_texture_binding_slot;
	};
}