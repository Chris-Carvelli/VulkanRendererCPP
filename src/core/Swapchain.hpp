#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class Device;

	typedef struct {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR>   present_modes;
	} SwapChainSupportDetails;

	class Swapchain {
	public:
		Swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkExtent2D extents);
		~Swapchain();
		Swapchain(const Swapchain&) = delete;
		Swapchain(Swapchain&& other) = delete;
		Swapchain& operator=(const Swapchain&) = delete;
		Swapchain& operator=(Swapchain&&) = delete;

		VkSwapchainKHR get_handle() const { return m_handle; };
		VkExtent2D get_extent() const { return m_extents; };
		VkFormat get_image_format() const { return m_image_format; };

		const std::vector<VkImageView>& get_image_views() const { return m_image_views; };

		void recreate(VkExtent2D extents);
	private:
		void create(VkExtent2D extents);
		void destroy();

		VkPhysicalDevice	m_physical_device_handle;
		VkDevice			m_device_handle;
		VkSurfaceKHR		m_surface_handle;

		VkSwapchainKHR m_handle{ VK_NULL_HANDLE };

		// swapchain support info
		SwapChainSupportDetails swap_chain_support;
		VkSurfaceFormatKHR surface_format;
		VkPresentModeKHR present_mode;
		

		// color outputs to be sent to the screen/window.
		// each RenderFrame will reference one of these (plus their own other attachments, like depth)
		std::vector<VkImage> m_images;
		std::vector<VkImageView> m_image_views;

		// info available for querying
		VkFormat m_image_format;

		VkExtent2D m_extents;
	};
}