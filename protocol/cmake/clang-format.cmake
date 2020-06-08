
find_program(CLANG_FORMAT_EXE NAMES "clang-format" DOC "Path to clang-format executable" )
if(NOT CLANG_FORMAT_EXE)
    message(STATUS "clang-format not found.")
else()
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
endif()

# Get all project files
file(GLOB_RECURSE ALL_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/desktop/*.c
        ${PROJECT_SOURCE_DIR}/desktop/*.h
        ${PROJECT_SOURCE_DIR}/desktop/*.m
        ${PROJECT_SOURCE_DIR}/server/*.c
        ${PROJECT_SOURCE_DIR}/server/*.h
        ${PROJECT_SOURCE_DIR}/server/*.m
        ${PROJECT_SOURCE_DIR}/fractal/*.c
        ${PROJECT_SOURCE_DIR}/fractal/*.h
        ${PROJECT_SOURCE_DIR}/fractal/*.m)

add_custom_target(
        clang-format
        COMMAND ${CLANG_FORMAT_EXE}
        -style=file
        -i
        ${ALL_SOURCE_FILES}
)
