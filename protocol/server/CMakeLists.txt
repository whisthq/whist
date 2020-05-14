#[[
################## FractalServer ##################
]]


set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/server/build64)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/server/build64)
add_executable(FractalServer
        main.c
        )


# FFMPEG, Threading and Dynamic linking libs

foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(STATUS ${LIB})
    target_link_libraries(FractalServer ${LIB})
endforeach(LIB)

# Threading and Dynamic link for linux/macosx only
# if we add other target platforms we may need to come back to this
if ( NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(FractalServer
            Threads::Threads
            ${CMAKE_DL_LIBS}
            )
endif ()


#message(FATAL_ERROR ${CMAKE_DL_LIBS})

#[[
################## Linking platform independent libs ##################
]]
target_link_libraries(FractalServer
        fractalCore
        fractalAudio
        fractalVideo
        fractalClipboard
        fractalCursor
        fractalInput
        fractalNetwork
        fractalUtils
        ${STATIC_SDL2}
        ${STATIC_OPENSSL}
        ${LIBMFX}
        )

#[[
################## LINUX ##################
]]
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(X11 REQUIRED
            Xfixes
            Xdamage
            Xext
            Xtst)
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(FractalServer
            ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            ${X11_Xdamage_LIB}
            ${X11_Xext_LIB}
            ${X11_Xtst_LIB}
            asound
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
    target_link_libraries(FractalServer
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



    set_property(TARGET FractalServer PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Link to system libs
    target_link_libraries(FractalServer
            libvcruntime
            libcmt
            kernel32
            ws2_32
            crypt32
            wldap32
            normaliz
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
            dxgi
            d3d11
            winspool
            comdlg32
            odbc32
            odbccp32
            ucrt
            vcruntime
            msvcrt
            msvcmrt
            libcpmt
            msvcprt
            avrt
            d3dcompiler
            dxguid
            crypt32
            shlwapi
            shcore
            )

endif()