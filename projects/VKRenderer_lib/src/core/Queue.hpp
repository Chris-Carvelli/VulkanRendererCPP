#pragma once

#include <vulkan/vulkan.h>

namespace vkc {
	class Device;

	class Queue {
	public:
		Queue(Device& device, uint32_t family_index, VkQueueFamilyProperties properties, VkBool32 can_present, uint32_t index);

		Queue(const Queue&) = default;

		Queue(Queue&& other);

		~Queue();

		/*void submit(const HPPCommandBuffer& command_buffer,  VkFence fence) const;

		 VkResult present(const  VkPresentInfoKHR& present_infos) const;*/
	private:
		Device& m_device;

		VkQueue m_handle;

		uint32_t m_family_index{ 0 };

		uint32_t m_index{ 0 };

		VkBool32 m_can_present = false;

		VkQueueFamilyProperties m_properties{};
	};
}