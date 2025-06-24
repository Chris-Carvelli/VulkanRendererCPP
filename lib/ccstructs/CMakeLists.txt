file(GLOB_RECURSE src "*.c")

add_library(CC_structs STATIC ${src})

target_include_directories(CC_structs PRIVATE ".")
