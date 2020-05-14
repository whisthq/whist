#[[
################## FractalClient ##################
]]


set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/desktop/build${arch})
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/desktop/build${arch})
add_executable(FractalClient
        main.c
        main.h
        video.c
        audio.c
        sdl_utils.c)


# FFMPEG, Threading and Dynamic linking libs

foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(STATUS ${LIB})
    target_link_libraries(FractalClient ${LIB})
endforeach(LIB)

# Threading and Dynamic link for linux/macosx only
# if we add other target platforms we may need to come back to this
if ( NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(FractalClient
            Threads::Threads
            ${CMAKE_DL_LIBS}
            )
endif ()


#message(FATAL_ERROR ${CMAKE_DL_LIBS})

#[[
################## Static Libraries we supply ##################
]]
find_library(STATIC_SDL2 SDL2 PATHS "${CMAKE_SOURCE_DIR}/lib/${arch}/SDL2/${CMAKE_SYSTEM_NAME}")
if(NOT STATIC_SDL2)
    message(FATAL_ERROR "Library STATIC_SDL2 was not found! ${CMAKE_SOURCE_DIR}/lib/${arch}/SDL2/${CMAKE_SYSTEM_NAME}")
endif()

find_library(STATIC_OPENSSL NAMES libcrypto crypto PATHS "${CMAKE_SOURCE_DIR}/lib/${arch}/openssl/${CMAKE_SYSTEM_NAME}")
#message(ERROR_FATAL ${CMAKE_SOURCE_DIR}/lib/${arch}/openssl/${CMAKE_SYSTEM_NAME}/)
if(NOT STATIC_OPENSSL)
    message(FATAL_ERROR "Library openssl was not found! ${CMAKE_SOURCE_DIR}/lib/${arch}/openssl/${CMAKE_SYSTEM_NAME}")
endif()

message(STATUS ${STATIC_OPENSSL})
#[[
################## Linking platform independent libs ##################
]]
target_link_libraries(FractalClient
        ${STATIC_SDL2}
        ${STATIC_OPENSSL}
        fractalCore
        fractalAudio
        fractalVideo
        fractalClipboard
        fractalCursor
        fractalInput
        fractalNetwork
        fractalUtils

        )

#[[
################## LINUX ##################
]]
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(X11 REQUIRED
            Xfixes)
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(FractalClient
            ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            m
            )
endif()


#[[
################## MACOS ##################
]]
# untested
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_C_COMPILER clang)
    set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
    target_link_libraries(FractalClient
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
endif()

#[[
################## WINDOWS ##################
]]
if(${WIN32})
    #compiler flags, TODO debug vs release
    SET(CMAKE_C_FLAGS  " -DWIN32_LEAN_AND_MEAN -DUNICODE -DWIN32 -DWIN32 /W4 /MT /MP" )

    #Linked flags: LNK4099 is when the linker cannot find the .pdb for debugging symbols
    # this comes up when we used precompiled libraries which we do not have .pdb files for
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099	")

    #libmfx, windows specific static lib
    find_library(LIBMFX libmfx HINTS ${CMAKE_SOURCE_DIR}/lib/${arch}/mfx/${CMAKE_SYSTEM_NAME}/)
    target_link_libraries(FractalClient ${LIBMFX})

    set_property(TARGET FractalClient PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Link to system libs
    target_link_libraries(FractalClient
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
            shcore
            )

endif()