#[[
################## FractalClient ##################
]]


set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/desktop/build64)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/desktop/build64)
add_executable(FractalClient
        main.c
        main.h
        video.c
        audio.c
        sdl_utils.c)


# FFMPEG, Threading and Dynamic linking libs
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(FractalClient
        PkgConfig::LIBAV
        Threads::Threads
        ${CMAKE_DL_LIBS}
        )

#[[
################## Static Libraries we supply ##################
]]
find_library(STATIC_SDL2 SDL2 HINTS ${CMAKE_SOURCE_DIR}/lib/${arch}/SDL2/${CMAKE_SYSTEM_NAME}/)
if(NOT STATIC_SDL2)
    message(FATAL_ERROR "Library STATIC_SDL2 was not found! ${CMAKE_SOURCE_DIR}/lib/${arch}/SDL2/${CMAKE_SYSTEM_NAME}/")
endif()

find_library(STATIC_OPENSSL crypto HINTS ${CMAKE_SOURCE_DIR}/lib/${arch}/openssl/${CMAKE_SYSTEM_NAME}/)
if(NOT STATIC_OPENSSL)
    message(FATAL_ERROR "Library openssl was not found! ${CMAKE_SOURCE_DIR}/lib/${arch}/openssl/${CMAKE_SYSTEM_NAME}/")
endif()
message(STATUS ${STATIC_SDL2})
message(STATUS ${STATIC_OPENSSL})
#[[
################## Linking platform independent libs ##################
]]
target_link_libraries(FractalClient
        m
        fractalAudio
        fractalVideo
        fractalCore
        fractalClipboard
        fractalCursor
        fractalInput
        fractalNetwork
        fractalUtils
        ${STATIC_SDL2}
        ${STATIC_OPENSSL}
        )

#[[
################## LINUX ##################
]]
if (${UNIX} AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(X11 REQUIRED
            Xfixes)
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(FractalClient
            ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            )
endif()


#[[
################## MACOS ##################
]]
if (${UNIX} AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_C_COMPILER clang)
    set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
    find_library(OSX_LIBS
            objc
            iconv
            OpenGL
            VideoToolbox
            CoreMedia
            CoreAudio
            AudioToolbox
            IOKit
            ForceFeedback
            Carbon
            AppKit
            Metal
            QuartzCore
            )
    target_link_libraries(FractalClient ${OSX_LIBS})
endif()

#[[
################## WINDOWS ##################
]]
if(${WIN32})
    find_library(WIN_LIBS
        libvcruntime
        libucrt
        libcmt
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
        shcore)
    target_link_libraries(FractalClient ${WIN_LIBS})
endif()