project(CC_STRUCTS)
set(CMAKE_C_STANDARD "99")

##SET (CMAKE_C_FLAGS_DEBUG_INIT "-g -O0")

if(MSVC)
    message("424243: msvc")
     add_compile_options(
         "-maes"
    )
    add_compile_options(
        "-Wall"
        "-Wextra"
        "-Werror"
    )
    add_compile_options(
        #important warnings
        "-Wno-vla" # we want ONLY CONSTANT-SIZED ARRAYS, to avoid using #defines
        "-Wno-gnu-pointer-arith" # not sure what is the reason behind this, sounds like void* should be the size of a pointer variable


        # style warning
        "-Wno-extra-semi-stmt"                   # would be nice to have this one enabled, but if we don't put the
                                                 # extra ; after macros VS screwes up indentation in subsequent lines
        "-Wno-gnu-zero-variadic-macro-arguments" # for assert log msg, could just be a string without extra params
        "-Wno-declaration-after-statement"       # ooooold C style
        "-Wno-newline-eof"

        # warning not possible to be ridden of
        "-Wno-unsafe-buffer-usage"
    )
    add_compile_options("/W4" "$<$<CONFIG:RELEASE>:/O2>")
else()
    add_compile_options("-Wall" "-Wextra" "-Werror" "$<$<CONFIG:RELEASE>:-O3>")
endif()

file(GLOB_RECURSE src "*.c")

add_library(CC_structs STATIC ${src})

target_include_directories(CC_structs PRIVATE ".")