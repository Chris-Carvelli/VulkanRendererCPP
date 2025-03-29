#pragma once

#include <vulkan/vulkan.h>

#include <core/RenderContext.hpp>
#include <core/VertexData.h>

#include <vector>

namespace vkc {
	namespace Drawcall {
		typedef struct {
			VkBuffer vertex_buffer;
			VkDeviceMemory vertexbuffer_memory;
			VkBuffer index_buffer;
			VkDeviceMemory index_buffer_memory;
			uint32_t indices_count;
		} ModelDataGPU;

		struct DrawcallData {
			vkc::RenderPass* obj_render_pass;
			vkc::Pipeline*   obj_pipeline;
			DataUniformModel data_uniform_model;
			uint32_t idx_data_attributes;
		};

		void add_drawcall(DrawcallData data);
		const std::vector<DrawcallData>& get_drawcalls();
		void clear_drawcalls();

		VkImageView get_pipeline_image_view();
		ModelDataGPU get_model_data(uint32_t index);

		void createTextureImage(TMP_Assets::TextureData& texture_data, VkDevice device, RenderContext* obj_render_context);
		void createModelBuffers(uint32_t model_index, VkDevice device, vkc::RenderContext* obj_render_context);

		void destroy_resources(VkDevice device);
	}
}