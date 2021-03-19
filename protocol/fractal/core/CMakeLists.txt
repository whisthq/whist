file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)

# Write git revision #define to a file
# Copy_if_different to only trigger a fractal.v change when the contents of the file actually changes
add_custom_target(git_revision ALL
        VERBATIM
        COMMAND ${CMAKE_COMMAND} -E echo_append "#define FRACTAL_GIT_REVISION " > ${CMAKE_BINARY_DIR}/include/fractal.sha.v
        COMMAND git rev-parse --short HEAD >> ${CMAKE_BINARY_DIR}/include/fractal.sha.v
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/include/fractal.sha.v ${CMAKE_BINARY_DIR}/include/fractal.v
        )

add_library(fractalCore STATIC
        fractal.h
        fractal.c
        fractalgetopt.h
        fractalgetopt.c)
# Ensure that fractal.v is updated prior to compiling fractalCore
add_dependencies(fractalCore git_revision)

set_property(TARGET fractalCore PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
