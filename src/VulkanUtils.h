#pragma once

#include <vulkan/vulkan.h>

#include <cc_logger.h>

// TODO stringification should be moved to debug-only classes
#include <vulkan/vk_enum_string_helper.h>

#define CC_VK_CHECK(y) {			\
	VkResult err = y;				\
									\
	if(err) {						\
		CC_EXIT(err,				\
			"[Vulkan error %d] %s",	\
			err,					\
			string_VkResult(err)	\
		);							\
	}								\
}
