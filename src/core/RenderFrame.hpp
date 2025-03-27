#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class RenderContext;
	class Swapchain;

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
			VkExtent2D swapchain_extent
		);
		void handle_swapchain_recreation();

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

		void handle_swapchain_destruction();

		// TODO Buffer pool: a data structure to allow high-level structures to send per-frame data to the GPU (uniform buffers mostly)
	};
}