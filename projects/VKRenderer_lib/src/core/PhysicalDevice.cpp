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


		uint32_t queue_families_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &queue_families_count, NULL);
		m_queue_families.resize(queue_families_count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &queue_families_count, m_queue_families.data());
	}
	PhysicalDevice::~PhysicalDevice() {
		// TODO destroy physical device
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

	uint32_t PhysicalDevice::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const {
		for (uint32_t i = 0; i < m_memory_properties.memoryTypeCount; i++) {
			if (type_filter & (1 << i) && (m_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		CC_LOG(CC_ERROR, "failed to find suitable memory type!");
		return -1;
	}

	const QueueFamilyIndices PhysicalDevice::find_queue_families() const {
		QueueFamilyIndices indices = { 0 };

		// from tutorial: "It's not really possible to use a magic value to indicate the nonexistence of a queue family,
		// since any value of uint32_t could in theory be a valid queue family index including 0"
		// but the we have `graphicsFamily < queueFamilyCount`???
		// -1 seems a perfectly valid magic value in this case.
		indices.graphicsFamily = -1;
		for (int i = 0; i < m_queue_families.size(); ++i)
		{
			if (m_queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = i;

			VkBool32 presentSupport = 0;
			vkGetPhysicalDeviceSurfaceSupportKHR(m_handle, i, m_surface, &presentSupport);

			if (presentSupport)
				indices.presentFamily = i;

			if (qfi_isComplete(&indices))
				break;
		}

		return indices;
	}

	void PhysicalDevice::print_info() {
		CC_PRINT(CC_IMPORTANT, "%s", m_properties.deviceName);
		CC_PRINT(CC_VERBOSE,   "%s", string_VkPhysicalDeviceType(static_cast<VkPhysicalDeviceType>(m_properties.deviceType)));
		
		CC_PRINT(CC_INFO, "");
		CC_PRINT(CC_INFO, "Memory Heaps");
		CC_PRINT(CC_INFO, "Id\tSize\tFlags");
		for (int i = 0; i < m_memory_properties.memoryHeapCount; ++i)
		{
			VkMemoryHeap& heap = m_memory_properties.memoryHeaps[i];
			const uint32_t BUFFER_SIZE = 16;
			char buf[BUFFER_SIZE] = { 0 };
			format_size(heap.size, buf, BUFFER_SIZE);
			CC_PRINT(CC_VERBOSE, "%2d\t%s\t%s", i, buf, string_VkMemoryHeapFlags(heap.flags).c_str());
		}

		CC_PRINT(CC_INFO, "");
		CC_PRINT(CC_INFO, "Memory Types");
		CC_PRINT(CC_INFO, "Heap\tFlags");
		for(int i = 0; i < m_memory_properties.memoryTypeCount; ++i)
		{
			VkMemoryType& type = m_memory_properties.memoryTypes[i];
			CC_PRINT(CC_VERBOSE, "%4d\t%s", type.heapIndex, string_VkMemoryPropertyFlags(type.propertyFlags).c_str());
		}

		CC_PRINT(CC_INFO, "");
		CC_PRINT(CC_INFO, "Queue Families");
		CC_PRINT(CC_INFO, "Id\tQueue Count\tFlags");
		for (int i = 0; i < m_queue_family_properties.size(); ++i)
		{
			VkQueueFamilyProperties queueFamilyProperty = m_queue_family_properties[i];
			CC_PRINT(CC_VERBOSE, "%2d\t%11d\t%s", i, queueFamilyProperty.queueCount, string_VkQueueFlags(queueFamilyProperty.queueFlags).c_str());
		}
	}
}