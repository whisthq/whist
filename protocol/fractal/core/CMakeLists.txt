file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)

# Write git revision #define to a file
# Copy_if_different to only trigger a fractal.v change when the contents
# of the file actually changes
add_custom_target(git_revision ALL
        VERBATIM
        COMMAND ${CMAKE_COMMAND} -E echo_append "#define WHIST_GIT_REVISION " > ${CMAKE_BINARY_DIR}/include/fractal.sha.v
        COMMAND git rev-parse --short HEAD >> ${CMAKE_BINARY_DIR}/include/fractal.sha.v
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/include/fractal.sha.v ${CMAKE_BINARY_DIR}/include/fractal.v
        )

add_library(whistCore STATIC
        fractal_memory.h
        fractal_memory.c
        fractal.h
        fractal.c
        whistgetopt.h
        whistgetopt.c
        fractal_frame.h
        fractal_frame.c
        )

# Ensure that fractal.v is updated prior to compiling whistCore
add_dependencies(whistCore git_revision)

set_property(TARGET whistCore PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
