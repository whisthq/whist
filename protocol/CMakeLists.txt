#[[
################## FractalProtocol ##################
This is the root CmakeLists.txt for the protocol repo
We define various project level settings here as well as include the subdirectories of the repo.
We also look for the ffmpeg libraries here.
To support WSL running with ubuntu 18.04, which only has cmake v1.10 in its repositories we use v1.10,
if you want to compile on windows with MSVC set it to 1.15 and uncomment the line setting the policy CMP0091
Additional VERBOSE message output is a feature of cmake v3.15 and later, if you use an earlier version the verbose messages
will be printed regardless of whether verbose output is set to ON or OFF.
]]

cmake_minimum_required(VERSION 3.15)
#3.15 required for CMP0091
#We need CMP0091 to select the MSVC_RUNTIME_LIBRARY
#See https://cmake.org/cmake/help/v3.15/prop_tgt/MSVC_RUNTIME_LIBRARY.html
cmake_policy(SET CMP0091 NEW)

#set default compiler flags
set (CMAKE_USER_MAKE_RULES_OVERRIDE "cmake/CompilerOptions.cmake")

project(FractalProtocol)

set(META_PROJECT_NAME        "FractalProtocol")
set(META_PROJECT_DESCRIPTION "CMake Project for the Fractal client protocol clients on mac, linux and windows
                              and servers on windows and linux.")
set(META_AUTHOR_ORGANIZATION "Fractal")
set(META_AUTHOR_DOMAIN       "https://github.com/fractalcomputers/protocol")

set(CMAKE_C_STANDARD 99)

# verbose cmake outputs, useful for debugging CMake issues such as linking
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Set up clang-format
# clang-format can be called with make clang-format
include(cmake/clang-format.cmake)
# A small module which sets the default build type to debug if a .git is present and release otherwise.
include(cmake/default-build.cmake)

option(BUILD_SERVER "build FractalServer" ON)
option(BUILD_CLIENT "build FractalClient" ON)

# For debug builds log every thing, for release builds log warnings or higher
if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    add_definitions(-DLOG_LEVEL=5)
elseif(${CMAKE_BUILD_TYPE} MATCHES Release)
    add_definitions(-DLOG_LEVEL=2)
endif()

# static analysis during build defaults to off, but we set to on in github actions
# you can also use this locally if you like, but will need to install cppcheck.
OPTION(USE_CPPCHECK OFF)
find_program(CPPCHECK cppcheck HINTS $ENV{PROGRAMFILES}/cppcheck)
if (CPPCHECK)
    message(STATUS "found cppcheck")
else()
    message(STATUS "cppcheck not found")
endif()

if (CPPCHECK AND ${USE_CPPCHECK} MATCHES "ON")
    message(STATUS CPPCHECKON)
    set(CMAKE_C_CPPCHECK "${CPPCHECK}"
            "--template=cppcheck:{file}:{line}:{severity}: cppcheck: {file}:{line} {message}"
            "--enable=performance,portability"
            "--verbose"
            "--std=c99"
            "--suppress=shiftTooManyBitsSigned"
            "--inline-suppr"
            )
    # exclude specific files
    set(CPPCHECK_EXCLUDES
            "${CMAKE_SOURCE_DIR}/fractal/clipboard/clipboard_osx.m" # cppcheck not supported on Objective-C files
            )
endif()


#detect if we are building 64bit or 32bit
if (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
    set(arch 64)
else()
    set(arch 32)
endif()


# ffmpeg: using our provided libs on mac and windows and system on linux.
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(FFMPEG_LIBS libswresample.3.dylib;libavcodec.58.dylib;libavdevice.58.dylib;libavfilter.7.dylib;
                    libavformat.58.dylib;libswscale.5.dylib;libavutil.56.dylib;libpostproc.55.dylib)
else()
    set(FFMPEG_LIBS swresample;avcodec;avdevice;avfilter;avformat;swscale;avutil;postproc)
endif()

set(FFMPEG_LIBS_PATHS "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    FOREACH(LIB ${FFMPEG_LIBS})
        message(VERBOSE "Looking for lib ${LIB}")
        set(LIB_VAR "LIB_${LIB}")
        find_library(${LIB_VAR} NAMES ${LIB})
        message(VERBOSE "found lib ${LIB_VAR}")
        LIST(APPEND FFMPEG_LIBS_PATHS ${${LIB_VAR}})
    endforeach(LIB)
    message(VERBOSE "FFMPEG PATHS ${FFMPEG_LIBS_PATHS}")
    include_directories(include/ffmpeg)
else()
    FOREACH(LIB ${FFMPEG_LIBS})
        message(STATUS "Looking for lib ${LIB}")
        set(LIB_VAR "LIB_${LIB}")
        find_library(${LIB_VAR}
                NAMES ${LIB}
                PATHS lib/${arch}/ffmpeg/${CMAKE_SYSTEM_NAME}
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
    include_directories(include/ffmpeg)
endif ()


#[[
################## Libraries we supply ##################
]]
# the NO_* paramaters tell cmake to only look for our included libs and not system wide ones.
find_library(STATIC_SDL2 NAMES SDL2 PATHS "${CMAKE_SOURCE_DIR}/lib/${arch}/SDL2/${CMAKE_SYSTEM_NAME}"
        NO_DEFAULT_PATH
        NO_CMAKE_ENVIRONMENT PATH
        NO_CMAKE_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_FIND_ROOT_PATH)
if(NOT STATIC_SDL2)
    message(FATAL_ERROR "Library STATIC_SDL2 was not found! ${CMAKE_SOURCE_DIR}/lib/${arch}/SDL2/${CMAKE_SYSTEM_NAME}")
endif()

find_library(STATIC_OPENSSL NAMES crypto libcrypto PATHS "${CMAKE_SOURCE_DIR}/lib/${arch}/openssl/${CMAKE_SYSTEM_NAME}/"
        NO_DEFAULT_PATH
        NO_CMAKE_ENVIRONMENT PATH
        NO_CMAKE_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_FIND_ROOT_PATH)

if(NOT STATIC_OPENSSL)
    message(FATAL_ERROR "Library openssl was not found! ${CMAKE_SOURCE_DIR}/lib/${arch}/openssl/${CMAKE_SYSTEM_NAME}/")
endif()

find_library(LIBMFX NAMES libmfx mfx PATHS ${CMAKE_SOURCE_DIR}/lib/${arch}/mfx/${CMAKE_SYSTEM_NAME}/
        NO_DEFAULT_PATH
        NO_CMAKE_ENVIRONMENT PATH
        NO_CMAKE_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_FIND_ROOT_PATH)

if((NOT LIBMFX AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
    message(FATAL_ERROR "Library LIBMFX was not found! ${CMAKE_SOURCE_DIR}/lib/${arch}/mfx/${CMAKE_SYSTEM_NAME}/")
endif()

#Set OSX compiler and SDK globally
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_C_COMPILER clang)
    set(CMAKE_OSX_SYSROOT /Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13) # Require MacOS 10.13 or up, to ensure LibObj-C is standard
endif()


# Tell CMake where to look for includes
include_directories(fractal)
include_directories(include)
include_directories(include/SDL2)
include_directories(include/ffmpeg)


#Here is where we tell cmake to look in subdirectories for further CMakeLists.txt
add_subdirectory(fractal)
if (${BUILD_CLIENT} MATCHES "ON")
    message(STATUS building client)
    add_subdirectory(desktop)
endif()
if (${BUILD_SERVER} MATCHES "ON" AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin") #Server does not ever build on macos
    message(STATUS building server)
    add_subdirectory(server)
endif()
