#include "Instance.hpp"

#include <cc_logger.h>
#include <VulkanUtils.h>

#include <algorithm>

namespace vkc {
	// load function addresses
	namespace
	{
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;

		// manual load of necessary function addresses
		void loadVkFuncAddr(VkInstance instance) {
			vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
			vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
			vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
		}
	}
	
	// validation layers debug reports
	namespace
	{
#ifdef ENABLE_VALID_LAYERS

		VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data)
		{
			// Log debug message
			if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			{
				//CC_LOG(WARNING, "%d - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
				CC_LOG(WARNING, callback_data->pMessage);

			}
			else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			{
				//CC_LOG(ERROR, "%d - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
				CC_LOG(ERROR, callback_data->pMessage);
			}
			return VK_FALSE;
		}

		static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*type*/,
			uint64_t /*object*/, size_t /*location*/, int32_t /*message_code*/,
			const char* layer_prefix, const char* message, void* /*user_data*/)
		{
			if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
			{
				CC_LOG(ERROR, "{}: {}", layer_prefix, message);
			}
			else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
			{
				CC_LOG(WARNING, "{}: {}", layer_prefix, message);
			}
			else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
			{
				CC_LOG(WARNING, "{}: {}", layer_prefix, message);
			}
			else
			{
				CC_LOG(LOG, "{}: {}", layer_prefix, message);
			}
			return VK_FALSE;
		}
#endif

		bool validate_layers(const std::vector<const char*>& required,
			const std::vector<VkLayerProperties>& available)
		{
			for (auto layer : required)
			{
				bool found = false;
				for (auto& available_layer : available)
				{
					if (strcmp(available_layer.layerName, layer) == 0)
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					CC_LOG(ERROR, "Validation Layer {} not found", layer);
					return false;
				}
			}

			return true;
		}
	}        // namespace
	
	// enabling of extensions and layers
	namespace
	{
		bool enable_extension(
			const char* requested_extension,
			const std::vector<VkExtensionProperties>& available_extensions,
			std::vector<const char*>& enabled_extensions
		)
		{
			bool is_available = false;
			for(auto const& available_extension : available_extensions)
				if (strcmp(requested_extension, available_extension.extensionName) == 0) {
					is_available = true;
					break;
				}

			if (!is_available)
			{
				CC_LOG(LogType::WARNING, "Extension %s not available", requested_extension);
				return false;
			}

			bool is_already_enabled = false;
			for (auto const& enabled_extension : enabled_extensions)
				if (strcmp(requested_extension, enabled_extension) == 0) {
					is_already_enabled = true;
					break;
				}

			if (!is_already_enabled)
			{
				CC_LOG(LogType::VERBOSE, "Extension %s available, enabling it", requested_extension);
				enabled_extensions.emplace_back(requested_extension);
			}

			return true;
		}

		bool enable_layer(
			const char* requested_layer,
			const std::vector<VkLayerProperties>& available_layers,
			std::vector<const char*>& enabled_layers
		) {
			bool is_available = false;
			for (auto const& available_layer : available_layers)
				if (strcmp(requested_layer, available_layer.layerName) == 0) {
					is_available = true;
					break;
				}

			if (!is_available)
			{
				CC_LOG(LogType::WARNING, "Extension %s not available", requested_layer);
				return false;
			}

			bool is_already_enabled = false;
			for (auto const& enabled_extension : enabled_layers)
				if (strcmp(requested_layer, enabled_extension) == 0) {
					is_already_enabled = true;
					break;
				}

			if (!is_already_enabled)
			{
				CC_LOG(LogType::VERBOSE, "Extension %s available, enabling it", requested_layer);
				enabled_layers.emplace_back(requested_layer);
			}

			return true;
		}
	}        // namespace

	Instance::Instance(
		const char* application_name,
		std::vector<const char*> extensions_requested,
		std::vector<const char*> extensions_optional,
		std::vector<const char*> layers_requested,
		std::vector<const char*> layers_optional,
		std::vector<VkLayerSettingEXT> layer_settings_required
	)
	{
		// TODO error handling
		uint32_t instance_extension_count;

		CC_VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr))

		std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
		CC_VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()))
		
#ifdef ENABLE_VALID_LAYERS
		// Check if VK_EXT_debug_utils is supported, which supersedes VK_EXT_Debug_Report
		const bool has_debug_utils = enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, available_instance_extensions, m_enabled_extensions);
		bool has_debug_report = false;

		if (!has_debug_utils)
		{
			has_debug_report = enable_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, available_instance_extensions, m_enabled_extensions);
			if (!has_debug_report)
			{
				CC_LOG(WARNING, "Neither of {} or {} are available; disabling debug reporting", VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}
		}
#endif

		// Specific surface extensions are obtained from  Window::get_required_surface_extensions
		// They are already added to requested_extensions by VulkanSample::prepare

		// Even for a headless surface a swapchain is still required
		enable_extension(VK_KHR_SURFACE_EXTENSION_NAME, available_instance_extensions, m_enabled_extensions);
		 
		// VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
		// which will be used for stats gathering where available.
		enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, available_instance_extensions, m_enabled_extensions);

		for (auto requested_extension : extensions_requested)
			if (!enable_extension(requested_extension, available_instance_extensions, m_enabled_extensions))
				CC_EXIT(-1, "Required instance extension %s not available, cannot run", requested_extension);

		for (auto requested_extension : extensions_optional)
			if (!enable_extension(requested_extension, available_instance_extensions, m_enabled_extensions))
				CC_LOG(LogType::WARNING, "Optional instance extension %s not available, some features may be disabled", requested_extension);

		uint32_t supported_layers_count;
		CC_VK_CHECK(vkEnumerateInstanceLayerProperties(&supported_layers_count, nullptr))

		std::vector<VkLayerProperties> supported_layers(supported_layers_count);
		CC_VK_CHECK(vkEnumerateInstanceLayerProperties(&supported_layers_count, supported_layers.data()))

		std::vector<const char*> enabled_layers;

		for (auto const& requested_layer : layers_requested)
			if (!enable_layer(requested_layer, supported_layers, enabled_layers))
				CC_EXIT(-1, "Required layer {} not available, cannot run", requested_layer);

		for (auto const& requested_layer : layers_requested)
			if (!enable_layer(requested_layer, supported_layers, enabled_layers))
				CC_LOG(LogType::WARNING, "Optional layer {} not available, some features may be disabled", requested_layer);

#ifdef ENABLE_VALID_LAYERS
		// NOTE: It's important to have the validation layer as the last one here!!!!
		//			 Otherwise, device creation fails !?!
		enable_layer("VK_LAYER_KHRONOS_validation", supported_layers, enabled_layers);
#endif

		VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
		app_info.pApplicationName	= application_name;
		app_info.applicationVersion	= 0;
		app_info.pApplicationName	= "VKC";
		app_info.engineVersion		= 0;
		app_info.apiVersion			= API_VERSION;

		VkInstanceCreateInfo instance_info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instance_info.pApplicationInfo = &app_info;

		instance_info.enabledExtensionCount = (uint32_t)m_enabled_extensions.size();
		instance_info.ppEnabledExtensionNames = m_enabled_extensions.data();

		instance_info.enabledLayerCount = enabled_layers.size();
		instance_info.ppEnabledLayerNames = enabled_layers.data();

#ifdef ENABLE_VALID_LAYERS
		VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		VkDebugReportCallbackCreateInfoEXT debug_report_create_info = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
		if (has_debug_utils)
		{
			debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

			instance_info.pNext = &debug_utils_create_info;
		}
		else if (has_debug_report)
		{
			debug_report_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			debug_report_create_info.pfnCallback = debug_callback;

			instance_info.pNext = &debug_report_create_info;
		}
#endif

		VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo{ VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT };

		// If layer settings are defined, then activate the sample's required layer settings during instance creation
		if (layer_settings_required.size() > 0)
		{
			layerSettingsCreateInfo.settingCount = static_cast<uint32_t>(layer_settings_required.size());
			layerSettingsCreateInfo.pSettings = layer_settings_required.data();
			layerSettingsCreateInfo.pNext = instance_info.pNext;
			instance_info.pNext = &layerSettingsCreateInfo;
		}

		// Create the Vulkan instance
		CC_VK_CHECK(vkCreateInstance(&instance_info, nullptr, &m_handle))

		//// CHRIS: what does this do? Here it defaults to static dispatcher (who does not have an init method),
		////	instead of dynamic one
		//// initialize the Vulkan-Hpp default dispatcher on the instance
		//VULKAN_HPP_DEFAULT_DISPATCHER.init(m_handle);

		//// CHRIS: here we have the caveat with HPP vulkan: looks like an additional dependency is needed,
		////	to handle some (eventual?) calls that are not yet ported to HPP in vulkan core
		//// Need to load volk for all the not-yet Vulkan-Hpp calls
		//volkLoadInstance(handle);

		loadVkFuncAddr(m_handle);

#ifdef ENABLE_VALID_LAYERS
			if (has_debug_utils)
				CC_VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_handle, &debug_utils_create_info, nullptr, &m_debug_utils_messenger))
			else if (has_debug_report)
				CC_VK_CHECK(vkCreateDebugReportCallbackEXT(m_handle, &debug_report_create_info, nullptr, &m_debug_report_callback))
#endif

		query_gpus();
	}

	Instance::~Instance() {
		
#ifdef ENABLE_VALID_LAYERS
		if (m_debug_utils_messenger != VK_NULL_HANDLE)
			vkDestroyDebugUtilsMessengerEXT(m_handle, m_debug_utils_messenger, NULL);
		if (m_debug_report_callback != VK_NULL_HANDLE)
			vkDestroyDebugReportCallbackEXT(m_handle, m_debug_report_callback, NULL);
#endif
		vkDestroyInstance(m_handle, NULL);
	}

	void Instance::print_info() const {
		CC_PRINT(LOG, "GPUs");
		for (auto& gpu : m_gpus)
			gpu->print_info();
	}

	void Instance::query_gpus() {
		uint32_t physical_devices_count;
		vkEnumeratePhysicalDevices(m_handle, &physical_devices_count, nullptr);

		std::vector<VkPhysicalDevice> physical_devices(physical_devices_count);
		vkEnumeratePhysicalDevices(m_handle, &physical_devices_count, physical_devices.data());

		if (physical_devices.empty())
			CC_EXIT(-1, "Couldn't find a physical device that supports Vulkan.")

		// Create gpus wrapper objects from the VkPhysicalDevice's
		for (auto& physical_device : physical_devices)
			m_gpus.push_back(std::make_unique<PhysicalDevice>(*this, physical_device));
	}

	const vkc::PhysicalDevice& Instance::get_selected_gpu() const {
		CC_ASSERT(!m_gpus.empty(), "No available gpu");
		return *m_gpus[0];
	};
}
 
