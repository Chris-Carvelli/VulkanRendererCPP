#include "RenderContext.hpp"

namespace vkc {
	// TODO fix these long constructor? (dependency injection good I guess, not sure about this)
	RenderContext::RenderContext(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkExtent2D extents)
	{
		m_swapchain = std::make_unique<Swapchain>(physical_device, device, surface, extents);

		// TODO setup render frames
		// samples do it in prepare, so probably not here
	}
}