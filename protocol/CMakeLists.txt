cmake_minimum_required(VERSION 3.16)
project(protocol)

set(META_PROJECT_NAME        "protocol")
set(META_PROJECT_DESCRIPTION "CMake Project Template")
set(META_AUTHOR_ORGANIZATION "Fractal Computers")
set(META_AUTHOR_DOMAIN       "https://github.com/fractalcomputers/protocol")


set(CMAKE_C_STANDARD 99)

if (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
    set(arch 64)
else()
    set(arch 32)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
        )

include_directories(fractal)
include_directories(include)


add_subdirectory(fractal)
add_subdirectory(desktop)
#add_subdirectory(server)
