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
		static Instance* TMP_singleton_instance;

	public:
		static Instance* TMP_get_singleton_instance() { return TMP_singleton_instance; };

		Instance(
			const char* application_name,
			std::vector<const char*> extensions_requested,
			std::vector<const char*> extensions_optional,
			std::vector<const char*> layers_requested,
			std::vector<const char*> layers_optional,
			std::vector<VkLayerSettingEXT> layer_settings_required
		);
		~Instance();

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

		void set_surface(VkSurfaceKHR surface) {
			// FIXME terrible hack because we don't handle GPU selection properly
			m_gpus[0]->set_surface(surface);
		};

		vkc::PhysicalDevice& get_selected_gpu() const;

		void add_object_debug_name(uint64_t object, VkObjectType object_type, VkDevice device, const char* debug_name);

		void begin_cmd_buffer_util_label(VkCommandBuffer buffer, const char* debug_name, float color[4] = VK_NULL_HANDLE);
		void add_cmd_buffer_util_label(VkCommandBuffer buffer, const char* debug_name, float color[4] = VK_NULL_HANDLE);
		void end_cmd_buffer_util_label(VkCommandBuffer buffer);

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
		VkDebugUtilsMessengerEXT m_debug_utils_messenger = VK_NULL_HANDLE;

		/**
		 * @brief The debug report callback
		 */
		VkDebugReportCallbackEXT m_debug_report_callback = VK_NULL_HANDLE;
#endif // VKC_DEBUG || ENABLE_VALID_LAYERS

	};

}