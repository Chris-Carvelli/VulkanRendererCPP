#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class Instance;

	class PhysicalDevice {
	public:
		PhysicalDevice(const vkc::Instance& instance, VkPhysicalDevice physical_device);
		PhysicalDevice() = delete;
		PhysicalDevice(const PhysicalDevice&) = delete;
		PhysicalDevice(PhysicalDevice&&) = delete;
		PhysicalDevice& operator=(const PhysicalDevice&) = delete;
		PhysicalDevice& operator=(PhysicalDevice&&) = delete;

		const VkPhysicalDevice get_handle() const { return m_handle; }

		VkBool32 is_present_supported(VkSurfaceKHR surface, uint32_t queue_family_index) const;

		/// <summary>
		/// temporary function to show Vulkan info on console
		/// </summary>
		void print_info();

		const std::vector<VkQueueFamilyProperties>& PhysicalDevice::get_queue_family_properties() const
		{
			return m_queue_family_properties;
		}

		const VkPhysicalDeviceFeatures get_requested_features() const
		{
			return m_requested_features;
		}

	private:
		const Instance& m_instance;

		VkPhysicalDevice m_handle{ nullptr };

		VkPhysicalDeviceFeatures m_features;

		std::vector< VkExtensionProperties> m_device_extensions;

		VkPhysicalDeviceProperties m_properties;

		VkPhysicalDeviceMemoryProperties m_memory_properties;

		std::vector<VkQueueFamilyProperties> m_queue_family_properties;

		VkPhysicalDeviceFeatures m_requested_features;

		// not handling extension features at the moment,
		// need to understand them better
	};
}