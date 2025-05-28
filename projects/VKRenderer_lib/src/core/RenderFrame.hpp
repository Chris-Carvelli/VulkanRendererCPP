#pragma once

#include <vulkan/vulkan.h>
#include <core/VertexData.h>

#include <vector>

namespace vkc {
	class RenderContext;
	class Swapchain;
	class Pipeline;

	namespace Drawcall {
		struct DrawcallData;
		struct DebugDrawcallData;
	}

	class RenderFrame {
	public:
		RenderFrame(
			VkDevice device, // FIXME may not be needed here
			RenderContext* render_context,
			VkCommandBuffer command_buffer
		);
		~RenderFrame();

		void render(
			VkSwapchainKHR swapchain,
			VkQueue queue_render,
			VkQueue queue_present,
			uint32_t frame_index,
			VkExtent2D swapchain_extent,
			DataUniformFrame ubo,
			const std::vector<Drawcall::DrawcallData>& drawcalls,
			const std::vector<Drawcall::DebugDrawcallData>& debug_drawcalls
		);

		void wait_fence() {
			vkWaitForFences(
				m_device,
				1,
				&m_fence_in_flight,
				VK_TRUE,
				UINT64_MAX
			);
		}

	private:
		// references
		vkc::RenderContext* m_render_context;
		VkDevice m_device;

		// command buffer to be recorded for this frame
		VkCommandBuffer m_command_buffer;

		// synchronization primitives
		VkFence m_fence_in_flight;
		VkSemaphore m_semaphore_render_finished;
		VkSemaphore m_semaphore_image_available;


		// TODO Buffer pool: a data structure to allow high-level structures to send per-frame data to the GPU (uniform buffers mostly)
	};
}