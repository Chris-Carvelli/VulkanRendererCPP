#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkc {
	class RenderPass {
	private:
		std::vector<uint32_t> m_attachments_input;
		std::vector<uint32_t> m_attachments_output;
	};
}