include_directories(include)

message("424243: ${LIBRARIES_SOURCE_PATH}")
include_directories(#${TARGET_NAME} PRIVATE
        ${LIBRARIES_SOURCE_PATH}/vulkan/include
		)
file(GLOB target_src "src/*.cpp" )
add_library(imgui STATIC ${target_src} "src/")