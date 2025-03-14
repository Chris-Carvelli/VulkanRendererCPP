#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class RenderFrame {
	private:
		// command buffer to be recorded for this frame
		VkCommandBuffer m_command_buffer;

		VkDescriptorSet m_descriptor_set;

		// synchronization primitives
		VkFence m_fence_in_flight;
		VkSemaphore m_semaphore_render_finished;
		VkSemaphore m_semaphore_image_available;

		// TODO Buffer pool: a data structure to allow high-level structures to send per-frame data to the GPU (uniform buffers mostly)

		// All the attachments needed for the current "Pipeline"
		// (all the various renderpasses), both inputs AND outputs.
		// One of these might be a color attachment to be rendered on screen,
		// in which case it need to be retrieved from the swapchain
		// (will probably be supplied by the RenderContext who own this)
		std::vector<VkImage> m_images;
		std::vector<VkImageView> m_image_views;

		// these are create info structures, may contain superfluous info?
		std::vector<VkAttachmentDescription> m_attachments;

		// CHRIS not sure we want these here, these are supposed to be defaults,
		// overridden by render passes?

		// indexes in m_images
		std::vector<uint32_t> m_attachments_input;

		// indexes in m_images
		std::vector<uint32_t> m_attachments_output;
	};
}