#pragma once

#include <vulkan/vulkan.h>

#include "Queue.hpp"

#include <vector>
#include <memory>

namespace vkc {
	class PhysicalDevice;
	class Queue;

	class Device {
	public:
		Device(
			const PhysicalDevice&    gpu,
			VkSurfaceKHR             surface,
			std::vector<const char*> extensions_requested = {},
			std::vector<const char*> extensions_desired = {}
		);
		~Device();

		/*Device(PhysicalDevice& gpu,
			VkDevice& vulkan_device,
			VkSurfaceKHR    surface);*/

		Device() = delete;
		Device(const Device&) = delete;
		Device(Device&&) = delete;
		Device& operator=(const Device&) = delete;
		Device& operator=(Device&&) = delete;

		VkDevice get_handle() const { return m_handle; }

	private:
		VkDevice m_handle;

		const PhysicalDevice & m_gpu;

		// TODO probably not needed. Check and remove
		VkSurfaceKHR m_surface{ nullptr };

		//std::unique_ptr<vkb::core::HPPDebugUtils> debug_utils;

		std::vector<const char*> m_enabled_extensions{};

		// array of families, each family has an array of queues
		std::vector<std::vector<Queue>> m_queues;

		/*/// A command pool associated to the primary queue
		std::unique_ptr<vkb::core::HPPCommandPool> command_pool;

		/// A fence pool associated to the primary queue
		std::unique_ptr<vkb::HPPFencePool> fence_pool;*/
	};
}