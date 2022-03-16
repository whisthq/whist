file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)

# Write git revision #define to a file
# Copy_if_different to only trigger a whist.v change when the contents
# of the file actually changes
add_custom_target(git_revision ALL
        VERBATIM
        COMMAND ${CMAKE_COMMAND} -E echo_append "#define WHIST_GIT_REVISION " > ${CMAKE_BINARY_DIR}/include/whist.sha.v
        COMMAND git rev-parse --short=15 HEAD >> ${CMAKE_BINARY_DIR}/include/whist.sha.v
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/include/whist.sha.v ${CMAKE_BINARY_DIR}/include/whist.v
        )

add_library(whistCore STATIC
        whist_memory.h
        whist_memory.c
        whist.h
        whist.c
        whist_frame.h
        whist_frame.c
        features.c
        error_codes.c
        )

# Ensure that whist.v is updated prior to compiling whistCore
add_dependencies(whistCore git_revision)

set_property(TARGET whistCore PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
