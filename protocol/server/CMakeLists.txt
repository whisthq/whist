#[[
################## FractalServer ##################
]]

add_executable(FractalServer
        main.c
        )

set_target_properties(FractalServer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/server/build${arch}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/server/build${arch})

#Print out compile flags in verbose mode of CMake
message(VERBOSE "c base flags ${CMAKE_C_FLAGS}")
message(VERBOSE "c release flags ${CMAKE_C_FLAGS_RELEASE}")
message(VERBOSE "c debug flags ${CMAKE_C_FLAGS_DEBUG}")


# FFMPEG, Threading and Dynamic linking libs
foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(STATUS ${LIB})
    target_link_libraries(FractalServer ${LIB})
endforeach(LIB)

# Threading and Dynamic link for linux/macosx only
# if we add other target platforms we may need to come back to this
if ( NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#    set(THREADS_PREFER_PTHREAD_FLAG ON)
#    find_package(Threads REQUIRED)
    target_link_libraries(FractalServer
#            Threads::Threads
            ${CMAKE_DL_LIBS}
            )
endif ()



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
    if(NOT X11_LIBRARIES)
        message(FATAL_ERROR "Library X11_LIBRARIES was not found!")
    endif()
    if(NOT X11_Xfixes_LIB)
        message(FATAL_ERROR "Library X11_Xfixes_LIB was not found!")
    endif()
    if(NOT X11_Xdamage_LIB)
        message(FATAL_ERROR "Library X11_Xdamage_LIB was not found!")
    endif()
    if(NOT X11_Xext_LIB)
        message(FATAL_ERROR "Library X11_Xext_LIB was not found!")
    endif()

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
################## WINDOWS ##################
]]
if(${WIN32})
    #Linked flags: LNK4099 is when the linker cannot find the .pdb for debugging symbols
    # this comes up when we used precompiled libraries which we do not have .pdb files for
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099	")

    #sets the MSVC runtime library as multithreaded and adds debug symbols if is a debug build
    #see https://cmake.org/cmake/help/v3.15/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html#variable:CMAKE_MSVC_RUNTIME_LIBRARY
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
            DbgHelp
            )

endif()