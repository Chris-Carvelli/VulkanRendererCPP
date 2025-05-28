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

		const uint32_t size_uniform_data_frame;
		const uint32_t size_uniform_data_material;

		const uint32_t size_push_constant_model;
		const uint32_t vertex_binding_descriptors_count;

		const uint32_t vertex_attribute_descriptors_count;
		uint32_t texture_slots_count;

		const uint8_t flags;
		// fixed pipeline config
		const VkPrimitiveTopology topology      = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		const VkCullModeFlags face_culling_mode = VK_CULL_MODE_FRONT_BIT;
		const VkCompareOp compare_op            = VK_COMPARE_OP_LESS;
	};

	const uint32_t PIPELINE_CONFIG_ID_SKYBOX = 0;
	const uint32_t PIPELINE_CONFIG_ID_PBR    = 1;
	const uint32_t PIPELINE_CONFIG_ID_UNLIT  = 2;
	const PipelineConfig PIPELINE_CONFIGS[] = {
		{
			.vert_path = "res/shaders/skybox.vert.spv",
			.frag_path = "res/shaders/skybox.frag.spv",
			.size_uniform_data_frame    = sizeof(DataUniformFrame),
			.size_uniform_data_material = 0,
			.size_push_constant_model   = 0,
			.texture_slots_count        = 1,
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
			.texture_slots_count        = 4,
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
			.texture_slots_count        = 1,
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
			const PipelineConfig* config
		);
		~Pipeline();

		const PipelineConfig* get_obj_config() const { return m_config; };

		VkPipeline            get_handle()                          const { return m_handle; };
		VkPipelineLayout      get_handle_layout()                   const { return m_handle_pipeline_layout; };
		VkDescriptorSetLayout get_handle_descriptor_set_layout()    const { return m_handle_descriptor_set_layout; };

		VkBuffer              get_handle_uniform_buffer(uint32_t i) const {
			CC_ASSERT(i < m_uniform_buffers.size(), "idx out of bounds");
			return m_uniform_buffers[i];
		}

		void update_uniform_buffer(void* ubo, uint32_t current_frame);

	private:
		void create_descriptor_set_layout();
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
		const PipelineConfig* m_config;

		// frame data
		std::vector<VkBuffer>			m_uniform_buffers;
		std::vector<VkDeviceMemory>		m_uniform_buffers_memory;
		std::vector<void*>				m_uniform_buffers_mapped;
	};
}