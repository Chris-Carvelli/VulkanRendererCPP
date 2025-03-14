#include "PhysicalDevice.hpp"

#include "Instance.hpp"

#include <VulkanUtils.h>
#include <cc_logger.h>

// TODO stringification should be moved to debug-only classes
#include <vulkan/vk_enum_string_helper.h>

namespace vkc {
	PhysicalDevice::PhysicalDevice(const Instance& instance, VkPhysicalDevice physical_device)
		: m_instance(instance)
		, m_handle(physical_device)
		, m_requested_features{ 0 }
	{
		vkGetPhysicalDeviceFeatures(physical_device, &m_features);
		vkGetPhysicalDeviceProperties(physical_device, &m_properties);
		vkGetPhysicalDeviceMemoryProperties(physical_device, &m_memory_properties);

		uint32_t queue_family_properties_count;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, nullptr);
		m_queue_family_properties = std::vector<VkQueueFamilyProperties>(queue_family_properties_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, m_queue_family_properties.data());

		uint32_t device_extension_properties_count;
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &device_extension_properties_count, nullptr);
		m_device_extensions = std::vector<VkExtensionProperties>(device_extension_properties_count);
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &device_extension_properties_count, m_device_extensions.data());
	}

	VkBool32 PhysicalDevice::is_present_supported(VkSurfaceKHR surface, uint32_t queue_family_index) const
	{
		VkBool32 present_supported{ VK_FALSE };

		if (surface != VK_NULL_HANDLE)
		{
			CC_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(m_handle, queue_family_index, surface, &present_supported));
		}

		return present_supported;
	}

	bool PhysicalDevice::is_extension_available(const char* extension_desired) const {
		for (const VkExtensionProperties& extension_supported : m_device_extensions) {
			// TODO_OPTIMIZATION: we could sort m_device_extension for binary search
			if (strcmp(extension_supported.extensionName, extension_desired) == 0)
				return true;
		}

		return false;
	}

	void PhysicalDevice::print_info() {
		CC_PRINT(IMPORTANT, "%s", m_properties.deviceName);
		CC_PRINT(VERBOSE,   "%s", string_VkPhysicalDeviceType(static_cast<VkPhysicalDeviceType>(m_properties.deviceType)));
		
		CC_PRINT(LOG, "");
		CC_PRINT(LOG, "Memory Heaps");
		CC_PRINT(LOG, "Id\tSize\tFlags");
		for (int i = 0; i < m_memory_properties.memoryHeapCount; ++i)
		{
			VkMemoryHeap& heap = m_memory_properties.memoryHeaps[i];
			char buf[16] = { 0 };
			formatSize(heap.size, buf);
			CC_PRINT(VERBOSE, "%2d\t%s\t%s", i, buf, string_VkMemoryHeapFlags(heap.flags).c_str());
		}

		CC_PRINT(LOG, "");
		CC_PRINT(LOG, "Memory Types");
		CC_PRINT(LOG, "Heap\tFlags");
		for(int i = 0; i < m_memory_properties.memoryTypeCount; ++i)
		{
			VkMemoryType& type = m_memory_properties.memoryTypes[i];
			CC_PRINT(VERBOSE, "%4d\t%s", type.heapIndex, string_VkMemoryPropertyFlags(type.propertyFlags).c_str());
		}

		CC_PRINT(LOG, "");
		CC_PRINT(LOG, "Queue Families");
		CC_PRINT(LOG, "Id\tQueue Count\tFlags");
		for (int i = 0; i < m_queue_family_properties.size(); ++i)
		{
			VkQueueFamilyProperties queueFamilyProperty = m_queue_family_properties[i];
			CC_PRINT(VERBOSE, "%2d\t%11d\t%s", i, queueFamilyProperty.queueCount, string_VkQueueFlags(queueFamilyProperty.queueFlags).c_str());
		}
	}
}