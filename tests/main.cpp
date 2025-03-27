#include <VKRenderer.hpp>

class TestRenderer : public VKRenderer {
	VkPipeline m_pipeline;
	VkPipelineLayout m_pipeline_layout;

	void init() {
		// TODO load assets
	}

	void record_command_buffer(VkCommandBuffer command_buffer) {

		//// geometry
		//VkBuffer vertex_buffer = get_vertex_buffer();
		//VkBuffer index_buffer = get_index_buffer();
		//int32_t indices_count = get_index_count();
		//const VkDescriptorSet* descriptor_set = get_descriptor_set();

		//VkBuffer vertexBuffers[] = { vertex_buffer };
		//VkDeviceSize offsets[] = { 0 };
		//vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
		//vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

		//vkCmdBindDescriptorSets(
		//	command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
		//	0, 1, descriptor_set,
		//	0, NULL
		//);
		//vkCmdDrawIndexed(command_buffer, indices_count, 1, 0, 0, 0);
	}
	void render() {
		//// application ============
		//updateUniformBuffer(currentFrame);
		//record_command_buffer(commandBuffers[currentFrame], imageIndex);

		//// application end ========
	}
};

int main() {
	TestRenderer app;

	app.run();

	return 0;
}