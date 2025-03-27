#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class RenderPass {
	private:
		VkRenderPass m_handle;
		VkRenderPass m_handle_framebuffer;

		std::vector<uint32_t> m_attachments_input;
		std::vector<uint32_t> m_attachments_output;
	};
}