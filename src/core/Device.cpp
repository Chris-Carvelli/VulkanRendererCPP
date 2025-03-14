#include "Device.hpp"

#include <VulkanUtils.h>
#include <core/PhysicalDevice.hpp>


namespace vkc
{
	Device::Device(
		const PhysicalDevice&    gpu,
		VkSurfaceKHR             surface,
		std::vector<const char*> extensions_requested,
		std::vector<const char*> extensions_desired
	)
		: m_gpu{gpu}
	{
		// Prepare the device queues
		uint32_t                             queue_family_properties_count = gpu.get_queue_family_properties().size();
		std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_family_properties_count, { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO });
		std::vector<std::vector<float>>      queue_priorities(queue_family_properties_count);

		for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties_count; ++queue_family_index)
		{
			const VkQueueFamilyProperties& queue_family_property = gpu.get_queue_family_properties()[queue_family_index];

			
			queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);

			VkDeviceQueueCreateInfo& queue_create_info = queue_create_infos[queue_family_index];

			queue_create_info.queueFamilyIndex = queue_family_index;
			queue_create_info.queueCount = queue_family_property.queueCount;
			queue_create_info.pQueuePriorities = queue_priorities[queue_family_index].data();
		}

		for (const char* extension_name : extensions_requested)
			if (gpu.is_extension_available(extension_name))
				m_enabled_extensions.emplace_back(extension_name);
			else
				CC_EXIT(-1, "Required extension %s not available on %s", extension_name, gpu.get_device_name());

		for (const char* extension_name : extensions_desired)
			if (gpu.is_extension_available(extension_name))
				m_enabled_extensions.emplace_back(extension_name);
			else
				CC_LOG(WARNING, "Desired extension %s not available on %s", extension_name, gpu.get_device_name());
		

		VkDeviceCreateInfo create_info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };

		// CHRIS is this even necessary?
		//// Latest requested feature will have the pNext's all set up for device creation.
		//create_info.pNext = gpu.get_extension_feature_chain();

		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.queueCreateInfoCount = queue_create_infos.size();
		create_info.enabledExtensionCount = m_enabled_extensions.size();
		create_info.ppEnabledExtensionNames = m_enabled_extensions.data();

		const auto requested_gpu_features = gpu.get_requested_features();
		create_info.pEnabledFeatures = &requested_gpu_features;

		CC_VK_CHECK(vkCreateDevice(gpu.get_handle(), &create_info, nullptr, &m_handle))

		m_queues.resize(queue_family_properties_count);

		for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties_count; ++queue_family_index)
		{
			const VkQueueFamilyProperties& queue_family_property = gpu.get_queue_family_properties()[queue_family_index];

			VkBool32 present_supported = gpu.is_present_supported(surface, queue_family_index);

			for (uint32_t queue_index = 0U; queue_index < queue_family_property.queueCount; ++queue_index)
			{
				m_queues[queue_family_index].emplace_back(*this, queue_family_index, queue_family_property, present_supported, queue_index);
			}
		}
	}
}