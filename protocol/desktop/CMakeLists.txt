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


#[[
################## Linking platform independent libs ##################
]]
target_link_libraries(FractalClient
        fractalCore
        fractalAudio
        fractalVideo
        fractalClipboard
        fractalNetwork
        fractalUtils
        ${STATIC_SDL2}
        ${STATIC_OPENSSL}
        )

#[[
################## LINUX ##################
]]
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(X11 REQUIRED
            Xfixes)
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(FractalClient fractalInput fractalCursor
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

    #MAC frameworks require library finding
    find_library(OpenGL REQUIRED)
    find_library(VideoToolbox REQUIRED)
    find_library(CoreMedia REQUIRED)
    find_library(CoreAudio REQUIRED)
    find_library(AudioToolbox REQUIRED)
    find_library(IOKit REQUIRED)
    find_library(ForceFeedback REQUIRED)
    find_library(Carbon REQUIRED)
    find_library(AppKit REQUIRED)
    find_library(Metal REQUIRED)
    find_library(Cocoa REQUIRED)
    find_library(QuartzCore REQUIRED)

    include_directories(
            ${OPENGL_INCLUDE_DIR}
            ${VIDEOTOOLBOX_INCLUDE_DIR}
            ${COREMEDIA_INCLUDE_DIR}
            ${COREAUDIO_INCLUDE_DIR}
            ${AUDIOTOOLBOX_INCLUDE_DIR}
            ${IOKIT_INCLUDE_DIR}
            ${FORCEFEEDBACK_INCLUDE_DIR}
            ${CARBON_INCLUDE_DIR}
            ${APPKIT_INCLUDE_DIR}
            ${METAL_INCLUDE_DIR}
            ${QUARTZCORE_INCLUDE_DIR}
            )
    target_link_libraries(FractalClient 
        ${OPENGL_LIBRARIES}
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
        )
endif()

#[[
################## WINDOWS ##################
]]
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    #compiler flags, TODO debug vs release
    SET(CMAKE_C_FLAGS  " -DWIN32_LEAN_AND_MEAN -DUNICODE -DWIN32 -DWIN32 /W4 /MT /MP" )

    #Linked flags: LNK4099 is when the linker cannot find the .pdb for debugging symbols
    # this comes up when we used precompiled libraries which we do not have .pdb files for
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099	")

    target_link_libraries(FractalClient fractalInput fractalCursor ${LIBMFX})

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