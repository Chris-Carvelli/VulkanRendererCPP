#pragma once

#include <vulkan/vulkan.h>

#include <AssetManager.hpp>
#include <core/Instance.hpp>
#include <core/RenderContext.hpp>
#include <core/VertexData.h>

#include <vector>

namespace vkc {
	namespace Drawcall {
		struct ModelDataGPU {
			VkBuffer vertex_buffer;
			VkDeviceMemory vertexbuffer_memory;
			VkBuffer index_buffer;
			VkDeviceMemory index_buffer_memory;
			uint32_t indices_count;
		};

		struct TextureDataGPU {
			VkImage image;
			VkDeviceMemory image_memory;
			VkImageView image_view;
		};

		struct DrawcallData {
			vkc::RenderPass* obj_render_pass;
			vkc::Pipeline*   obj_pipeline;
			DataUniformMaterial* data_uniform_material;
			uint32_t data_uniform_model_size;
			uint32_t idx_data_attributes;
			void* data_uniform_model;
		};

		void add_drawcall(DrawcallData data);
		const std::vector<DrawcallData>& get_drawcalls();
		void clear_drawcalls();

		VkImageView get_texture_image_view(uint32_t id);
		ModelDataGPU get_model_data(uint32_t index);

		void createTextureImage(
			Assets::IdAssetTexture texture_id,
			VkDevice device,
			vkc::RenderContext* obj_render_context
		);
		void createTextureCubemap(
			Assets::IdAssetTexture texture_id,
			VkDevice device,
			vkc::RenderContext* obj_render_context
		);

		void updateModelVertexBuffer(
			uint32_t model_index,
			void* vertex_buffer_content,
			uint32_t vertex_buffer_size,
			VkDevice device,
			vkc::RenderContext* obj_render_context
		);
		uint32_t createModelBuffers(
			uint32_t model_index,
			VkDevice device,
			vkc::RenderContext* obj_render_context
		);

		void destroy_resources(VkDevice device);

		void add_debug_name(uint32_t gpu_data_id, VkDevice device, vkc::Instance* obj_device, const char* debug_name);

		// ======================================================================
		// debug drawcalls
		// ======================================================================

		struct DebugDrawcallData {
			DataUniformModelDebug data_uniform_model;
			uint32_t idx_data_attributes;
		};

		void add_debug_cube(glm::vec3 pos, glm::vec3 rot, glm::vec3 size, glm::vec3 color);
		void add_debug_ray(glm::vec3 pos, glm::vec3 dir, float length, glm::vec3 color);
		const std::vector<DebugDrawcallData>& get_debug_drawcalls();
		void clear_debug_drawcalls();
	}
}