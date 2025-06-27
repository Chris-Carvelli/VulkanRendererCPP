# =========================================================
# Project Settings
# =========================================================
cmake_minimum_required(VERSION 3.10)
project(VulkanRendererCPP)

find_package(Vulkan REQUIRED)

set(CMAKE_C_STANDARD "11")
set(CMAKE_CXX_STANDARD "20")

# no stdlib
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

SET (CMAKE_CXX_FLAGS_DEBUG_INIT          "-g -O0")

set(ENABLE_VALID_LAYERS ON CACHE BOOL "Enable Validation Layers")

add_compile_definitions($<$<CONFIG:DEBUG>:VKC_DEBUG>)

if(ENABLE_VALID_LAYERS)
    add_definitions(-DENABLE_VALID_LAYERS)
endif(ENABLE_VALID_LAYERS)


MACRO(SUBDIRLIST result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

MACRO(FILELIST result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(NOT IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()


# =========================================================
# Libraries
# =========================================================

set(LIBRARIES_SOURCE_PATH "${CMAKE_SOURCE_DIR}/lib")

add_subdirectory(${CMAKE_SOURCE_DIR}/lib)


# =========================================================
# Projects
# =========================================================

## VKRenderer_lib
file(
    GLOB_RECURSE src
    "projects/VKRenderer_lib/*.cpp"
    "projects/VKRenderer_lib/*.cpp"
    "projects/AssetBaker/*.cpp" # TODO this is not great, we should split the AssetBaker in "design only" and "shared" functionality
)
add_library(VKRenderer_lib STATIC ${src})

target_include_directories(VKRenderer_lib PUBLIC "projects/VKRenderer_lib/src")
target_include_directories(VKRenderer_lib PUBLIC "projects/VKRenderer_lib/include")

target_include_directories(VKRenderer_lib PUBLIC
    projects/AssetBaker/include
    
	${LIBRARIES_SOURCE_PATH}/stb/include    # TODO we DO NOT want to include stb here, but we need it as long as
                                            # we need to carry along ALL of the AssetBaker

	${LIBRARIES_SOURCE_PATH}/imgui/include
	${LIBRARIES_SOURCE_PATH}/glm
    ${LIBRARIES_SOURCE_PATH}/vulkan/include
    ${LIBRARIES_SOURCE_PATH}/ccstructs
)

target_link_libraries(VKRenderer_lib Vulkan::Vulkan)
target_link_libraries(VKRenderer_lib imgui)
target_link_libraries(VKRenderer_lib assimp)
target_link_libraries(VKRenderer_lib CC_structs)

## Asset baker
file(GLOB_RECURSE src "projects/AssetBaker/*.cpp")
add_executable(AssetBaker ${src})

target_include_directories(AssetBaker PUBLIC "projects/AssetBaker/src")
target_include_directories(AssetBaker PUBLIC "projects/AssetBaker/include")

target_include_directories(AssetBaker PUBLIC
	# ${LIBRARIES_SOURCE_PATH}/imgui/include
    ${LIBRARIES_SOURCE_PATH}/vulkan/include # we need vulkan incude for flags and formats
                                            # we do NOT need to link vulkan because we are
                                            # not using any actual functionality
                                            # (this may change when auto-generating mipmaps)
	${LIBRARIES_SOURCE_PATH}/glm
	${LIBRARIES_SOURCE_PATH}/stb/include

    ${LIBRARIES_SOURCE_PATH}/ccstructs
    ${LIBRARIES_SOURCE_PATH}/assimp
)

target_link_libraries(AssetBaker Vulkan::Vulkan)
target_link_libraries(AssetBaker assimp)

# =========================================================
# tests
# =========================================================
FILELIST(tests "${CMAKE_CURRENT_LIST_DIR}/tests")
FOREACH(test_exe ${tests})
    set(TARGETNAME ${test_exe})
    add_executable(${TARGETNAME} "${CMAKE_CURRENT_LIST_DIR}/tests/${test_exe}")
    target_link_libraries(${TARGETNAME} VKRenderer_lib)
    target_include_directories(${TARGETNAME} PRIVATE "lib")
endforeach()

FILELIST(tests "${CMAKE_CURRENT_LIST_DIR}/tests_c")
FOREACH(test_exe ${tests})
    set(TARGETNAME ${test_exe})
    add_executable(${TARGETNAME} "${CMAKE_CURRENT_LIST_DIR}/tests_c/${test_exe}")
    target_link_libraries(${TARGETNAME} CC_structs)
    target_include_directories(${TARGETNAME} PRIVATE ${LIBRARIES_SOURCE_PATH}/ccstructs)
endforeach()