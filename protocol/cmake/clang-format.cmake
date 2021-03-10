find_program(
  CLANG_FORMAT_EXE
  NAMES "clang-format"
  DOC "Path to clang-format executable")
if(NOT CLANG_FORMAT_EXE)
  message(STATUS "clang-format not found.")
else()
  message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
endif()

# Get all project files
file(
  GLOB_RECURSE
  ALL_SOURCE_FILES
  CONFIGURE_DEPENDS
  ${PROJECT_SOURCE_DIR}/client/*.c
  ${PROJECT_SOURCE_DIR}/client/*.h
  ${PROJECT_SOURCE_DIR}/client/*.m
  ${PROJECT_SOURCE_DIR}/server/*.c
  ${PROJECT_SOURCE_DIR}/server/*.h
  ${PROJECT_SOURCE_DIR}/server/*.m
  ${PROJECT_SOURCE_DIR}/fractal/*.c
  ${PROJECT_SOURCE_DIR}/fractal/*.h
  ${PROJECT_SOURCE_DIR}/fractal/*.m
)

file(GLOB_RECURSE CLIENT_SOURCE_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/client/*.c
     ${PROJECT_SOURCE_DIR}/client/*.h ${PROJECT_SOURCE_DIR}/client/*.m)

file(GLOB_RECURSE SERVER_SOURCE_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/server/*.c
     ${PROJECT_SOURCE_DIR}/server/*.h CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/server/*.m)

file(GLOB_RECURSE FRACTAL_DIR_C_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/fractal/*.c)

file(GLOB_RECURSE FRACTAL_DIR_H_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/fractal/*.h)

file(GLOB_RECURSE FRACTAL_DIR_M_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/fractal/*.m)

add_custom_target(
  clang-format
  COMMAND ${CLANG_FORMAT_EXE} -style=file -i ${CLIENT_SOURCE_FILES}
  COMMAND ${CLANG_FORMAT_EXE} -style=file -i ${SERVER_SOURCE_FILES}
  COMMAND ${CLANG_FORMAT_EXE} -style=file -i ${FRACTAL_DIR_C_FILES}
  COMMAND ${CLANG_FORMAT_EXE} -style=file -i ${FRACTAL_DIR_H_FILES}
  COMMAND ${CLANG_FORMAT_EXE} -style=file -i ${FRACTAL_DIR_M_FILES}
)

