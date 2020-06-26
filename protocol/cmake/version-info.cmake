find_package(Git)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    INCLUDE(cmake/GetGitRevisionDescription.cmake)
    EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE SHORT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE)

    SET(REVISION ${SHORT_SHA} CACHE STRING "git short sha" FORCE)

    # only use the plugin to tie the configure state to the sha to force rebuilds
    # of files that depend on version.h
    include(cmake/GetGitRevisionDescription.cmake)
    get_git_head_revision(REFSPEC COMMITHASH)
else()
    message(WARNING "Git not found, cannot set version info")

    SET(REVISION "unknown")
endif()

# generate version.h
configure_file("${CMAKE_SOURCE_DIR}/fractal/core/version.h.in" "${CMAKE_SOURCE_DIR}/fractal/core/fractal.version" @ONLY)