#pragma once

#include <vulkan/vulkan.h>

#include "PhysicalDevice.hpp"

#include <vector>
#include <memory>

namespace vkc {
	class PhysicalDevice;

	class Instance {
	private:
		const uint32_t API_VERSION = VK_API_VERSION_1_0;

	public:
		Instance(
			const char* application_name,
			std::vector<const char*> extensions_requested,
			std::vector<const char*> extensions_optional,
			std::vector<const char*> layers_requested,
			std::vector<const char*> layers_optional,
			std::vector<VkLayerSettingEXT> layer_settings_required
		);

		Instance() = delete;
		Instance(const Instance&) = delete;
		Instance(Instance&&) = delete;
		Instance &operator=(const Instance&) = delete;
		Instance &operator=(Instance&&) = delete;

		/// <summary>
		/// temporary function to show Vulkan info on console
		/// </summary>
		void print_info() const;

		VkInstance get_handle() const { return m_handle; };

		const vkc::PhysicalDevice& get_selected_gpu() const;

	private:
		void query_gpus();

	private:
		VkInstance m_handle;

		std::vector<const char*> m_enabled_extensions;

		std::vector<std::unique_ptr<vkc::PhysicalDevice>> m_gpus;

#if defined(VKC_DEBUG) || defined(ENABLE_VALID_LAYERS)
		/**
		 * @brief Debug utils messenger callback for VK_EXT_Debug_Utils
		 */
		VkDebugUtilsMessengerEXT m_debug_utils_messenger;

		/**
		 * @brief The debug report callback
		 */
		VkDebugReportCallbackEXT m_debug_report_callback;
#endif // VKC_DEBUG || ENABLE_VALID_LAYERS

	};

}