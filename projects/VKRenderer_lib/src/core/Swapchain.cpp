#include "Swapchain.hpp"

#include <VulkanUtils.h>
#include <core/Device.hpp>

namespace vkc {
	// CHRIS: most of this methods ported from C, some raw pointers VS smart-pointers tension here
	namespace {
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
			SwapChainSupportDetails details = { };

			CC_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities))

				// formats
				uint32_t num_formats;
			CC_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, NULL))
				if (num_formats != 0) {
					details.formats.resize(num_formats);
					vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, details.formats.data());
				}

			// presetModes
			uint32_t num_resent_modes;
			CC_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_resent_modes, NULL))
				if (num_resent_modes != 0) {
					details.present_modes.resize(num_resent_modes);
					CC_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_resent_modes, details.present_modes.data()))
				}

			return details;
		}

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR* availableFormats, const uint32_t numAvailableFormats) {
			for (int i = 0; i < numAvailableFormats; ++i) {
				if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					return availableFormats[i];
			}

			return availableFormats[0];
		}

		VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR* availablePresentModes, const uint32_t numPresentModes) {
			for (uint32_t i = 0; i < numPresentModes; ++i) {
				if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
					return availablePresentModes[i];
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkExtent2D chooseSwapchainExtent(
			VkExtent2D request_extent,
			VkSurfaceCapabilitiesKHR& capabilities
		) {
			if (capabilities.currentExtent.width <= 4096)
				return capabilities.currentExtent;

			// surface that is bigger than the hardware driver but different from special value UINT32_MAX
			// can happen 
			if (capabilities.currentExtent.width != UINT32_MAX)
				return (VkExtent2D) { .width = 0, .height = 0 };

			CC_LOG(CC_WARNING, "[chooseSwapExtent] Vulkan gannot get exact window resolution!");

			// FIXME these are the wrong min and max. Try `vkGetPhysicalDeviceImageFormatProperties`
			request_extent.width = std::max(request_extent.width, capabilities.minImageExtent.width);
			request_extent.width = std::min(request_extent.width, capabilities.maxImageExtent.width);

			request_extent.height = std::max(request_extent.height, capabilities.minImageExtent.height);
			request_extent.height = std::min(request_extent.height, capabilities.maxImageExtent.height);

			return request_extent;
		}
	}

	Swapchain::Swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkExtent2D extents)
	{
		m_physical_device_handle = physical_device;
		m_device_handle = device;
		m_surface_handle = surface;

		create(extents);
	}

	Swapchain::~Swapchain()
	{
		destroy();
	}

	void Swapchain::recreate(VkExtent2D extents) {
		destroy();
		create(extents);
	}

	void Swapchain::create(VkExtent2D extents) {
		swap_chain_support = querySwapChainSupport(m_physical_device_handle, m_surface_handle);
		surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats.data(), swap_chain_support.formats.size());
		present_mode = chooseSwapPresentMode(swap_chain_support.present_modes.data(), swap_chain_support.present_modes.size());
		VkExtent2D extent = chooseSwapchainExtent(extents, swap_chain_support.capabilities);

		// image count
		uint32_t swapChainImagesCount = swap_chain_support.capabilities.minImageCount + 1;
		if (swap_chain_support.capabilities.maxImageCount > 0 && swapChainImagesCount > swap_chain_support.capabilities.maxImageCount) {
			swapChainImagesCount = swap_chain_support.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		createInfo.surface = m_surface_handle;
		createInfo.minImageCount = swapChainImagesCount;
		createInfo.imageFormat = surface_format.format;
		createInfo.imageColorSpace = surface_format.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		//// queue family indices. We could select the most appropriate queue family if we look for queue families
		//{
		//	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		//	createInfo.queueFamilyIndexCount = 0; // Optional
		//	createInfo.pQueueFamilyIndices = NULL; // Optional
		//}

		createInfo.preTransform = swap_chain_support.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = present_mode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		CC_VK_CHECK(vkCreateSwapchainKHR(m_device_handle, &createInfo, NULL, &m_handle))


		m_image_format = surface_format.format;
		m_extents = extent;

		// retrieve images
		// setup swapchain images
		uint32_t TMP_imageCountCapabilities = swapChainImagesCount;
		CC_VK_CHECK(vkGetSwapchainImagesKHR(m_device_handle, m_handle, &swapChainImagesCount, NULL))

		if (TMP_imageCountCapabilities != swapChainImagesCount)
			CC_LOG(CC_WARNING, "[swapchain] different swapChainImagesCount. Capabilities: %d. Actual: %d", TMP_imageCountCapabilities, swapChainImagesCount);

		m_images.resize(swapChainImagesCount);
		m_image_views.resize(swapChainImagesCount);
		CC_VK_CHECK(vkGetSwapchainImagesKHR(m_device_handle, m_handle, &swapChainImagesCount, m_images.data()))


		// create image views
		VkImageViewCreateInfo image_view_createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		image_view_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_createInfo.format = m_image_format;
		image_view_createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_createInfo.subresourceRange.baseMipLevel = 0;
		image_view_createInfo.subresourceRange.levelCount = 1;
		image_view_createInfo.subresourceRange.baseArrayLayer = 0;
		image_view_createInfo.subresourceRange.layerCount = 1;

		for(int i = 0; i < swapChainImagesCount; ++i)
		{
			image_view_createInfo.image = m_images[i];
			if (vkCreateImageView(m_device_handle, &image_view_createInfo, NULL, &m_image_views[i]) != VK_SUCCESS)
				CC_LOG(CC_ERROR, "failed to create image views");
		}
	}

	void Swapchain::destroy() {
		if (m_handle == VK_NULL_HANDLE)
			return;

		vkDeviceWaitIdle(m_device_handle);

		for (VkImageView handle_image_view : m_image_views)
			vkDestroyImageView(m_device_handle, handle_image_view, NULL);

		vkDestroySwapchainKHR(m_device_handle, m_handle, nullptr);
	}
}