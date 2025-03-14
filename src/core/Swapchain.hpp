#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class Device;

	enum ImageFormat
	{
		sRGB,
		UNORM
	};

	struct SwapchainProperties
	{
		VkSwapchainKHR                old_swapchain;
		uint32_t                      image_count{ 3 };
		VkExtent2D                    extent{};
		VkSurfaceFormatKHR            surface_format{};
		uint32_t                      array_layers;
		VkImageUsageFlags             image_usage;
		VkSurfaceTransformFlagBitsKHR pre_transform;
		VkCompositeAlphaFlagBitsKHR   composite_alpha;
		VkPresentModeKHR              present_mode;
	};

	class Swapchain {
	public:
		Swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkExtent2D extents);
		~Swapchain();
		Swapchain(const Swapchain&) = delete;
		Swapchain(Swapchain&& other) = delete;
		Swapchain& operator=(const Swapchain&) = delete;
		Swapchain& operator=(Swapchain&&) = delete;
	private:
		VkDevice m_device_handle;

		VkSwapchainKHR m_handle{ VK_NULL_HANDLE };

		// color outputs to be sent to the screen/window.
		// each RenderFrame will reference one of these (plus their own other attachments, like depth)
		std::vector<VkImage> m_images;

		// info available for querying
		VkFormat m_image_format;

		VkExtent2D m_extents;
	};
}