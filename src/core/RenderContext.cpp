#include "RenderContext.hpp"

#include <VulkanUtils.h>

namespace vkc {
	// TODO fix these long constructor? (dependency injection good I guess, not sure about this)
	RenderContext::RenderContext(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkExtent2D extents)
	{
		m_swapchain = std::make_unique<Swapchain>(physical_device, device, surface, extents);

		uint32_t num_frames_in_flight = 3;


		m_frames.resize(num_frames_in_flight);
		// TODO setup render frames
		// samples do it in prepare, so probably not here


		// setup draw cmd buffers
		m_cmd_buffers_draw.resize(num_frames_in_flight);
		VkCommandBufferAllocateInfo command_buffer_allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		command_buffer_allocate_info.commandPool = m_command_pool;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate_info.commandBufferCount = num_frames_in_flight;
		CC_VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, m_cmd_buffers_draw.data()))
	}
}