#pragma once

#include <vulkan/vulkan.h>

#include <core/Swapchain.hpp>
#include <core/RenderFrame.hpp>

#include <memory>


namespace vkc {
	class RenderContext {
	public:
		RenderContext(
			VkPhysicalDevice physical_device,
			VkDevice device,
			VkSurfaceKHR surface,
			VkExtent2D extents
		);

		uint32_t get_num_render_frames() const { return m_frames.size(); };
		VkCommandBuffer get_current_command_buffer();

	private:
		/// If swapchain exists, then this will be a present supported queue, else a graphics queue
		VkQueue m_queue;

		
		std::unique_ptr<vkc::Swapchain> m_swapchain;

		// struc SwapchainProperties, let's see what's needed here and where

		std::vector<std::unique_ptr<RenderFrame>> m_frames;

		// one per frame (should RenderFrame own its own buffer?)
		std::vector<VkCommandBuffer> m_cmd_buffers_draw;

		VkSemaphore m_acquired_semaphore;

		// pools
		VkCommandPool m_command_pool;
		VkDescriptorPool m_descriptor_pool;

		bool m_prepared{ false };

		/// Whether a frame is active or not
		bool m_frame_active{ false };

		/// Current active frame index
		uint32_t m_active_frame_index{ 0 };
	};
}