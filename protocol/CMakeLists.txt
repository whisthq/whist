#[[
###################################### Fractal Protocol ######################################
This is the root CmakeLists.txt for the protocol repository. We define various project level
settings here as well as include the subdirectories of the repository. We also look for the
protocol libraries needed here (FFmpeg, SDL, etc.).

To support WSL running with Ubuntu 18.04, which only has Cmake v3.10 in its repositories, manually
set cmake_minimum_required(VERSION 3.10) and comment cmake_policy(SET CMP0091 NEW), which requires
version 3.15.

If you want to compile on Windows with MSVC, set cmake_minimum_required(VERSION 3.15) and uncomment
cmake_policy(SET CMP0091 NEW).

Additional VERBOSE message output is a feature of Cmake v3.15 and later. If you use an earlier version,
the verbose messages will be printed regardless of whether verbose output is set to ON or OFF.
]]

# 3.15 required for CMP0091
# We need CMP0091 to select the MSVC_RUNTIME_LIBRARY
# We need CheckLanguage to call check_language
# See https://cmake.org/cmake/help/v3.15/prop_tgt/MSVC_RUNTIME_LIBRARY.html
cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
include(CheckLanguage)

project(FractalProtocol LANGUAGES C CXX)

if (CMAKE_CONFIGURATION_TYPES)
    message(FATAL_ERROR "Detected multi-configuration generator! This CMake file does not support multi-configuration generators.")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    check_language(CUDA)
    # CMAKE_CUDA_COMPILER will be set only if check_language(CUDA) succeeded
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        add_definitions(-DFRACTAL_CUDA_ENABLED)
    endif()
endif()

# Special work-around for getting include_directories(SYSTEM path) to work on MSVC
# Found from https://www.reddit.com/r/cpp/comments/elewps/msvc_isystem/
# Waiting on https://gitlab.kitware.com/cmake/cmake/-/issues/17904 to be merged into master cmake branch
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS
    "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/external:W0;/experimental:external>")

if(MSVC)
    set(CMAKE_INCLUDE_SYSTEM_FLAG_C "/external:I ")
endif()

# Add our compiler flags
include(cmake/CompilerOptions.cmake)

# Add rpath to look for shared libraries on linux
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_SKIP_BUILD_RPATH  FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH $ORIGIN)
endif()

# Add rpath to look for shared libraries on mac
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SKIP_BUILD_RPATH  FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH @executable_path)
endif()

set(META_PROJECT_NAME        "Protocol")
set(META_PROJECT_DESCRIPTION "CMake Project for the Fractal client protocol clients on mac, linux and windows
                              and servers on windows and linux.")
set(META_AUTHOR_ORGANIZATION "Fractal")
set(META_AUTHOR_DOMAIN       "https://github.com/fractal/fractal")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)

# verbose cmake outputs, useful for debugging CMake issues such as linking
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Set up clang-format
# clang-format can be called with make clang-format
include(cmake/clang-format.cmake)
# A small module which sets the default build type to debug if a .git is present and release otherwise.
include(cmake/default-build.cmake)

# For debug builds log every thing, for release builds log warnings or higher
if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    add_definitions(-DLOG_LEVEL=5)
elseif(${CMAKE_BUILD_TYPE} MATCHES Release)
    # For now, always show all logs
    add_definitions(-DLOG_LEVEL=5)
endif()
add_definitions(-D__ROOT_FILE__="${PROJECT_SOURCE_DIR}")

# CI will by-default by off, but we set to ON in github actions
# You can locally set to ON as well if you like, just make sure to have cppcheck installed
OPTION(CI OFF)

find_program(CPPCHECK cppcheck HINTS $ENV{PROGRAMFILES}/cppcheck)
if (CPPCHECK)
    message(STATUS "cppcheck found: ${CPPCHECK}")
else()
    message(STATUS "cppcheck not found")
endif()

if (CPPCHECK AND ${CI} MATCHES "TRUE" AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message(STATUS "cppcheck active")
    set(CMAKE_C_CPPCHECK "${CPPCHECK}"
            "--template=\"cppcheck: {file}:{line} ({severity}) {message}\""
            "--enable=performance,portability"
            "--verbose"
            "--std=c99"
            "-j 8" # Run in 8 threads
            "--error-exitcode=1" # Propagate cppcheck errors to cmake
            "--suppress=shiftTooManyBitsSigned"
            "--suppress=*:${CMAKE_SOURCE_DIR}/fractal/clipboard/clipboard_osx.m" # suppress all warnings for clipboard_osx.m since cppcheck isn't supported on Objective-C
            "--suppress=*:${CMAKE_SOURCE_DIR}/fractal/utils/lodepng.*" # suppress all lodepng warnings, since this is an external library that we should not touch
            "--inline-suppr" # Allow cppcheck-suppress comments in code
            )
else()
    message(STATUS "cppcheck inactive")
endif()

# export compile commands database - not needed if not running clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message(STATUS "Exporting compile commands for clang-tidy")

# detect if we are building 64bit or 32bit
if (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
    set(arch 64)
else()
    set(arch 32)
endif()

# Windows option to add icon (used primarily for client app compilation pipeline)
option(USE_CLIENT_APP_ICON "use client app icon in exe" OFF)

# Get Bash on all OSs
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Find git
    find_program(GIT git REQUIRED)
    # Find git's path
    get_filename_component(GIT_DIR ${GIT} DIRECTORY)
    # Look for gitbash based on git's path
    # NO_DEFAULT_PATH to avoid finding WSL bash, or any other bash
    find_program(BASH bash PATHS "${GIT_DIR}/../bin" NO_DEFAULT_PATH)

    if (NOT BASH)
        if(EXISTS "C:\\Windows\\System32\\wsl.exe")
            set(WSL "C:\\Windows\\System32\\wsl.exe")
            set(BASH "/bin/bash")
        else()
            message(FATAL_ERROR "Neither Git Bash nor WSL Bash were found! For WSL bash, make sure to create a symlink from C:\\Windows\\SysWOW64\\wsl.exe to C:\\Windows\\System32\\wsl.exe if you are on a 32-bit CMake install.")
        endif()
    endif()
else()
    set(BASH "/bin/bash")
endif()

# download AWS S3 binaries unless indicated otherwise
option(DOWNLOAD_BINARIES "download binaries from S3 via AWS CLI" ON)
if (DOWNLOAD_BINARIES)
    message(STATUS "Downloading binaries from AWS S3")
    if (WSL)
        set(download_args ${CMAKE_SYSTEM_NAME} "`wslpath ${CMAKE_BINARY_DIR}/client/build64`" "`wslpath ${CMAKE_BINARY_DIR}/server/build64`" "`wslpath ${CMAKE_BINARY_DIR}`" "`wslpath ${CMAKE_BINARY_DIR}/.download-binaries-cache`")
        execute_process(COMMAND ${WSL} ${BASH} "`wslpath ${CMAKE_CURRENT_SOURCE_DIR}/download-binaries.sh`" ${download_args}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE download_result)
    else()
        set(download_args ${CMAKE_SYSTEM_NAME} ${CMAKE_BINARY_DIR}/client/build64 ${CMAKE_BINARY_DIR}/server/build64 ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/.download-binaries-cache)
        execute_process(COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/download-binaries.sh ${download_args}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE download_result)
    endif()
    if (NOT ${download_result} EQUAL "0")
        message(FATAL_ERROR "Failed to download binaries from AWS S3: exitcode: ${download_result}")
    endif()
endif()

# FFmpeg: Use the ffmpeg libs in /lib/${arch}/ffmpeg
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(FFMPEG_LIBS libswresample.4.dylib;libavcodec.59.dylib;libavdevice.59.dylib;libavfilter.8.dylib;
                    libavformat.59.dylib;libswscale.6.dylib;libavutil.57.dylib;libpostproc.56.dylib)
else()
    set(FFMPEG_LIBS swresample;avcodec;avdevice;avfilter;avformat;swscale;avutil;postproc)
endif()

set(FFMPEG_LIBS_PATHS "")
FOREACH(LIB ${FFMPEG_LIBS})
    message(STATUS "Looking for lib ${LIB}")
    set(LIB_VAR "LIB_${LIB}")
    find_library(${LIB_VAR}
            NAMES ${LIB}
            PATHS ${CMAKE_BINARY_DIR}/lib/${arch}/ffmpeg/${CMAKE_SYSTEM_NAME}
            NO_DEFAULT_PATH
            NO_CMAKE_ENVIRONMENT PATH
            NO_CMAKE_PATH
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_FIND_ROOT_PATH)
    message(VERBOSE "found lib ${LIB_VAR}")
    LIST(APPEND FFMPEG_LIBS_PATHS ${${LIB_VAR}})
endforeach(LIB)
message(VERBOSE "FFMPEG PATHS ${FFMPEG_LIBS_PATHS}")


#[[
################## Libraries we supply ##################
]]
# the NO_* paramaters tell cmake to only look for our included libs and not system wide ones.
# For linux we use the system openssl/libcrypto. For mac and windows we bundle our own because is not included with the os
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_library(LIB_OPENSSL NAMES crypto libcrypto PATHS "${CMAKE_BINARY_DIR}/lib/${arch}/openssl/${CMAKE_SYSTEM_NAME}/"
            NO_DEFAULT_PATH
            NO_CMAKE_ENVIRONMENT PATH
            NO_CMAKE_PATH
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_FIND_ROOT_PATH)

    if (NOT LIB_OPENSSL)
        message(FATAL_ERROR "Library openssl was not found! ${CMAKE_BINARY_DIR}/lib/${arch}/openssl/${CMAKE_SYSTEM_NAME}/")
    endif()
else()
    find_package(OpenSSL REQUIRED)
endif()

find_library(LIB_SENTRY NAMES sentry PATHS "${CMAKE_BINARY_DIR}/lib/${arch}/sentry/${CMAKE_SYSTEM_NAME}"
        NO_DEFAULT_PATH
        NO_CMAKE_ENVIRONMENT_PATH
        NO_CMAKE_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_FIND_ROOT_PATH)
if (NOT LIB_SENTRY)
    message(FATAL_ERROR "Library sentry was not found! ${CMAKE_BINARY_DIR}/lib/${arch}/sentry/${CMAKE_SYSTEM_NAME}")
endif()

find_library(STATIC_SDL2 NAMES SDL2 SDL2-static PATHS "${CMAKE_BINARY_DIR}/lib/${arch}/SDL2/${CMAKE_SYSTEM_NAME}"
        NO_DEFAULT_PATH
        NO_CMAKE_ENVIRONMENT_PATH
        NO_CMAKE_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_FIND_ROOT_PATH)
if (NOT STATIC_SDL2)
    message(FATAL_ERROR "Library STATIC_SDL2 was not found! ${CMAKE_BINARY_DIR}/lib/${arch}/SDL2/${CMAKE_SYSTEM_NAME}")
endif()

find_library(LIBMFX NAMES libmfx mfx PATHS ${CMAKE_SOURCE_DIR}/lib/${arch}/mfx/${CMAKE_SYSTEM_NAME}/
        NO_DEFAULT_PATH
        NO_CMAKE_ENVIRONMENT_PATH
        NO_CMAKE_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_FIND_ROOT_PATH)

if ((NOT LIBMFX AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
    message(FATAL_ERROR "Library LIBMFX was not found! ${CMAKE_SOURCE_DIR}/lib/${arch}/mfx/${CMAKE_SYSTEM_NAME}")
endif()

# GoogleTests should be included before we set CMAKE_C_COMPILER to 'clang'.
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/master.zip
)
# From GoogleTests' official README: prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

#Set OSX compiler and SDK globally
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_C_COMPILER clang)
    if(DEFINED ENV{FRACTAL_OSX_SYSROOT})
        set(CMAKE_OSX_SYSROOT $ENV{FRACTAL_OSX_SYSROOT})
    else()
        set(CMAKE_OSX_SYSROOT /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk)
    endif()
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15) # deployment target must be <= than compiler SDK above
endif()

# Tell CMake where to look for includes
# Use SYSTEM flag to not get warnings from their files
include_directories(${CMAKE_SOURCE_DIR}) # For #include <fractal/**> semantics
include_directories(SYSTEM include)
include_directories(SYSTEM ${CMAKE_BINARY_DIR}/include)
include_directories(SYSTEM ${CMAKE_BINARY_DIR}/include/ffmpeg)

# Here is where we tell cmake to look in subdirectories for further CMakeLists.txt
add_subdirectory(fractal)

# Set variables to be used by the CMakeLists in the subdirectories
set(PLATFORM_INDEPENDENT_LIBS   fractalCore
    fractalAudio
    fractalVideo
    fractalClipboard
    fractalNetwork
    fractalLogging
    fractalUtils
    ${STATIC_SDL2}
    ${LIB_SENTRY}
)
set(MAC_SPECIFIC_CLIENT_LIBS       "-framework OpenGL"
    "-framework VideoToolbox"
    "-framework CoreMedia"
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework IOKit"
    "-framework ForceFeedback"
    "-framework Carbon"
    "-framework AppKit"
    "-framework Metal"
    "-framework QuartzCore"
    objc
    iconv
    ${LIB_OPENSSL}
)

# MSVC C/C++ Standard Library linking, based on Debug/Release
if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    set(WINDOWS_CORE_LIBS
        ucrtd
        libcmtd
        libcpmtd
        vcruntimed
        msvcrtd
        msvcmrtd
        msvcprtd
    )
elseif(${CMAKE_BUILD_TYPE} MATCHES Release)
    set(WINDOWS_CORE_LIBS
        ucrt
        libcmt
        libcpmt
        vcruntime
        msvcrt
        msvcmrt
        msvcprt
    )
endif()

# Windows Libs Shared by both server and client go here
set(WINDOWS_CORE_LIBS
    ${WINDOWS_CORE_LIBS}
    ${LIB_OPENSSL}
    oldnames # Needed by gtest for things like dup2/creat/etc
    kernel32
    gdi32
    winmm
    imm32
    shell32
    advapi32
    ole32
    oleaut32
    opengl32
    user32
    uuid
    version
    ws2_32
    shlwapi
    crypt32
    d3d11
    dxgi
    dxguid
    shcore
    DbgHelp
    setupapi
)

# Client Specific windows libs go here
set(WINDOWS_SPECIFIC_CLIENT_LIBS
    ${WINDOWS_CORE_LIBS}
)

add_subdirectory(client)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin") # Server does not make or compile on MacOS
    set(SERV "FractalServer")
    set(PLATFORM_INDEPENDENT_SERVER_LIBS
        fractalCore
        fractalAudio
        fractalVideo
        fractalClipboard
        fractalCursor
        fractalInput
        fractalNetwork
        fractalLogging
        fractalUtils
        ${STATIC_SDL2}
        ${LIB_SENTRY}
        ${LIBMFX}
    )
    # Server Specific windows libs go here
    set(WINDOWS_SPECIFIC_SERVER_LIBS
        ${WINDOWS_CORE_LIBS}
        wldap32
        normaliz
        winspool
        comdlg32
        odbc32
        odbccp32
        avrt
        d3dcompiler
    )
    add_subdirectory(server)
endif()
# Include the unit tests code
add_subdirectory(test)

if (WSL)
    message(STATUS "Native bash not found (only WSL), clang-tidy will not be an option")
else()
    add_custom_target(clang-tidy
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${BASH} ${CMAKE_SOURCE_DIR}/run-clang-tidy.sh -c
        DEPENDS FractalClient ${SERV})
endif()

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_target(docs
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND mkdir -p ${CMAKE_BINARY_DIR}/docs
        COMMAND cp ${CMAKE_SOURCE_DIR}/docs/Doxyfile ${CMAKE_BINARY_DIR}/docs
        COMMAND ${CMAKE_COMMAND} -E echo "OUTPUT_DIRECTORY = ${CMAKE_BINARY_DIR}/docs" >> ${CMAKE_BINARY_DIR}/docs/Doxyfile
        COMMAND doxygen ${CMAKE_BINARY_DIR}/docs/Doxyfile
        )
endif ()
