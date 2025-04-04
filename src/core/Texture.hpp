#pragma once

#include <vulkan/vulkan.h>

#include <map>

namespace vkc {
	struct Texture {
		VkImage m_image;
		VkDeviceMemory m_image_memory;
		VkImageView m_image_view;
	};

	class TextureStorage {
		std::map<VkImage, Texture> m_textures;
	};
}