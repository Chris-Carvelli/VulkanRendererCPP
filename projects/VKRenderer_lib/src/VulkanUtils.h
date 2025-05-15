#pragma once

#include <vulkan/vulkan.h>

#include <cc_logger.h>

// TODO stringification should be moved to debug-only classes
#include <vulkan/vk_enum_string_helper.h>

#include <fstream>
#include <vector>

#define CC_VK_CHECK(y) {			\
	VkResult err = y;				\
									\
	if(err) {						\
		CC_LOG(ERROR,				\
			"[Vulkan error %d] %s",	\
			err,					\
			string_VkResult(err)	\
		);							\
		assert(false);				\
	}								\
}


// temporary file reading
namespace TMP_VUlkanUtils {
	inline std::vector<char> read_file_binary(const char* path) {
		std::string s_path(path);

		std::ifstream myfile(s_path, std::ios::in | std::ios::binary);
		assert(myfile.is_open());
		return std::vector<char>(std::istreambuf_iterator<char>(myfile), {});
	}
}
