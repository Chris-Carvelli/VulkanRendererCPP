#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class Instance;

	typedef struct {
		int32_t graphicsFamily;
		int32_t presentFamily;
	} QueueFamilyIndices;

	class PhysicalDevice {
	public:
		PhysicalDevice(
			const vkc::Instance& instance,
			VkPhysicalDevice physical_device
		);
		~PhysicalDevice();
		PhysicalDevice() = delete;
		PhysicalDevice(const PhysicalDevice&) = delete;
		PhysicalDevice(PhysicalDevice&&) = delete;
		PhysicalDevice& operator=(const PhysicalDevice&) = delete;
		PhysicalDevice& operator=(PhysicalDevice&&) = delete;

		const VkPhysicalDevice get_handle() const { return m_handle; }

		// surface is needed to create graphics and present queue later, but
		// needs physical device to be created (this is why OOP sucks)
		// must remember to set surface ASAP afer creation!
		void set_surface(VkSurfaceKHR surface) { m_surface = surface; };

		VkBool32 is_present_supported(VkSurfaceKHR surface, uint32_t queue_family_index) const;
		
		uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

		const QueueFamilyIndices find_queue_families() const;

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

		bool is_extension_available(const char* extension_desired) const;

		const char* get_device_name() const { return m_properties.deviceName; };

		const VkPhysicalDeviceProperties& get_physical_device_properties() const { return m_properties; };

	private:
		const Instance& m_instance;

		VkPhysicalDevice m_handle{ nullptr };
		VkSurfaceKHR m_surface;

		VkPhysicalDeviceFeatures m_features;

		std::vector< VkExtensionProperties> m_device_extensions;

		VkPhysicalDeviceProperties m_properties;

		VkPhysicalDeviceMemoryProperties m_memory_properties;

		std::vector<VkQueueFamilyProperties> m_queue_family_properties;

		VkPhysicalDeviceFeatures m_requested_features;

		std::vector<VkQueueFamilyProperties> m_queue_families;

		int qfi_isComplete(QueueFamilyIndices* qfi) const {
			return
				qfi->graphicsFamily > -1 &&
				qfi->presentFamily > -1;
		}

		// not handling extension features at the moment,
		// need to understand them better
	};
}