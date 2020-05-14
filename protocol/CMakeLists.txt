cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0091 NEW)
project(protocol)

set(META_PROJECT_NAME        "protocol")
set(META_PROJECT_DESCRIPTION "CMake Project Template")
set(META_AUTHOR_ORGANIZATION "Fractal Computers")
set(META_AUTHOR_DOMAIN       "https://github.com/fractalcomputers/protocol")
set( CMAKE_VERBOSE_MAKEFILE on )
set(CMAKE_C_STANDARD 99)

if (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
    set(arch 64)
else()
    set(arch 32)
endif()

# ffmpeg: using our provided libs on mac and windows and system on linux.
set(FFMPEG_LIBS swresample;avcodec;avdevice;avfilter;avformat;swscale;avutil;postproc)
set(FFMPEG_LIBS_PATHS)

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
        message(VERBOSE "Looking for lib ${LIB}")
        set(LIB_VAR "LIB_${LIB}")
        find_library(${LIB_VAR}
                NAMES ${LIB}
                PATHS lib/${arch}/ffmpeg/Windows
                NO_DEFAULT_PATH
                NO_CMAKE_ENVIRONMENT PATH
                NO_CMAKE_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_FIND_ROOT_PATH)
        message(VERBOSE "found lib ${LIB_VAR}")
        LIST(APPEND FFMPEG_LIBS_PATHS ${${LIB_VAR}})
    endforeach(LIB)
    message(v "FFMPEG PATHS ${FFMPEG_LIBS_PATHS}")
    include_directories(include/ffmpeg)
endif ()

include_directories(fractal)
include_directories(include)
include_directories(include/SDL2)


add_subdirectory(fractal)
add_subdirectory(desktop)
add_subdirectory(server)
