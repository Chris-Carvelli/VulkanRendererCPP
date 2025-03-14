#include "Swapchain.hpp"

#include <VulkanUtils.h>
#include <core/Device.hpp>

// CHRIS: most of this methods ported from C, some raw pointers VS smart-pointers tension here
namespace {
    typedef struct {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   present_modes;
    } SwapChainSupportDetails;

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
        const VkExtent2D& min_image_extent,
        const VkExtent2D& max_image_extent
    ) {
        request_extent.width = std::max(request_extent.width, min_image_extent.width);
        request_extent.width = std::min(request_extent.width, max_image_extent.width);

        request_extent.height = std::max(request_extent.height, min_image_extent.height);
        request_extent.height = std::min(request_extent.height, max_image_extent.height);

        return request_extent;
    }
}


namespace vkc {
	Swapchain::Swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkExtent2D extents)
    {
        m_device_handle = device;

        // CHRIS do we really need this every time we re-create the swapchain? This is the same for all physical devices!
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physical_device, surface);
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats.data(), swapChainSupport.formats.size());
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.present_modes.data(), swapChainSupport.present_modes.size());

        // CHRIS this probably needs to be done every re-creation. Mostly triggered by window resizing
        VkExtent2D extent = chooseSwapchainExtent(extents, swapChainSupport.capabilities.minImageExtent, swapChainSupport.capabilities.maxImageExtent);

        // image count
        uint32_t swapChainImagesCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && swapChainImagesCount > swapChainSupport.capabilities.maxImageCount) {
            swapChainImagesCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = surface;
        createInfo.minImageCount = swapChainImagesCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        //// queue family indices. We could select the most appropriate queue family if we look for queue families
        //{
        //    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        //    createInfo.queueFamilyIndexCount = 0; // Optional
        //    createInfo.pQueueFamilyIndices = NULL; // Optional
        //}

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        CC_VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, NULL, &m_handle))


        m_image_format = surfaceFormat.format;
        m_extents = extent;

        // retrieve images
        // setup swapchain images
        uint32_t TMP_imageCountCapabilities = swapChainImagesCount;
        CC_VK_CHECK(vkGetSwapchainImagesKHR(device, m_handle, &swapChainImagesCount, NULL))

        if (TMP_imageCountCapabilities != swapChainImagesCount)
            CC_LOG(WARNING, "[swapchain] different swapChainImagesCount. Capabilities: %d. Actual: %d", TMP_imageCountCapabilities, swapChainImagesCount);

        m_images.resize(swapChainImagesCount);
        CC_VK_CHECK(vkGetSwapchainImagesKHR(device, m_handle, &swapChainImagesCount, m_images.data()))
	}

	Swapchain::~Swapchain()
	{
		if (m_handle != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(m_device_handle, m_handle, nullptr);
		}
	}
}