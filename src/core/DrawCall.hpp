#pragma once

#include <vulkan/vulkan.h>

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
			DataUniformModel data_uniform_model;
			uint32_t idx_data_attributes;
		};

		void add_drawcall(DrawcallData data);
		const std::vector<DrawcallData>& get_drawcalls();
		void clear_drawcalls();

		VkImageView get_texture_image_view(uint32_t id);
		ModelDataGPU get_model_data(uint32_t index);

		void createTextureImage(uint32_t texture_id, TMP_Assets::TextureData& texture_data, VkDevice device, vkc::RenderContext* obj_render_context);
		void createModelBuffers(uint32_t model_index, VkDevice device, vkc::RenderContext* obj_render_context);

		void destroy_resources(VkDevice device);
	}
}