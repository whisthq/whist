#[[
################## FractalClient ##################
]]

add_executable(FractalClient
        main.c
        main.h
        audio.c
        desktop_utils.c
        fractalgetopt.c
        network.c
        sdl_event_handler.c
        sdl_utils.c
        server_message_handler.c
        video.c
        )

# set the build directory
set_target_properties(FractalClient PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/desktop/build${arch}/${CMAKE_SYSTEM_NAME}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/desktop/build${arch}/${CMAKE_SYSTEM_NAME})

# move necessary files into the build directory
add_custom_command(TARGET FractalClient PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/desktop/loading $<TARGET_FILE_DIR:FractalClient>/loading)
add_custom_command(TARGET FractalClient PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/desktop/external_utils/${CMAKE_SYSTEM_NAME} $<TARGET_FILE_DIR:FractalClient>)

# Print out compile flags in verbose mode of CMake
message(VERBOSE "c base flags${CMAKE_C_FLAGS}")
message(VERBOSE "c release flags${CMAKE_C_FLAGS_RELEASE}")
message(VERBOSE "c debug flags${CMAKE_C_FLAGS_DEBUG}")

# FFmpeg, Threading and Dynamic linking libs
foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(STATUS ${LIB})
    target_link_libraries(FractalClient ${LIB})
endforeach(LIB)

# Threading and Dynamic link for linux/macosx only
# if we add other target platforms we may need to come back to this
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#    set(THREADS_PREFER_PTHREAD_FLAG ON)
#    find_package(Threads REQUIRED)
    target_link_libraries(FractalClient
#            Threads::Threads
            ${CMAKE_DL_LIBS}
            )
endif()

#[[
################## Linking platform independent libs ##################
]]
target_link_libraries(FractalClient
        fractalCore
        fractalAudio
        fractalCursor
        fractalVideo
        fractalClipboard
        fractalNetwork
        fractalUtils
        ${STATIC_SDL2}
        )

#[[
################## LINUX ##################
]]
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(X11 REQUIRED
            Xfixes)
    find_package(ALSA REQUIRED)
    include_directories(${X11_INCLUDE_DIR} ${ALSA_INCLUD_DIR})
    target_link_libraries(FractalClient fractalInput
            ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            ${ALSA_LIBRARIES}
            m
            OpenSSL::Crypto
            )
    target_link_libraries(FractalClient sentry)
endif()

#[[
################## MACOS ##################
]]
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(FractalClient
        "-framework OpenGL"
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
        ${STATIC_OPENSSL}
        )
endif()

#[[
################## WINDOWS ##################
]]
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message(STATUS c base flags${CMAKE_C_FLAGS})
    message(STATUS c release flags${CMAKE_C_FLAGS_RELEASE})
    message(STATUS c debug flags${CMAKE_C_FLAGS_DEBUG})
    message(STATUS c CI flags${CMAKE_C_FLAGS_CI})
    # Linked flags: LNK4099 is when the linker cannot find the .pdb for debugging symbols
    # this comes up when we used precompiled libraries which we do not have .pdb files for
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099	")

    target_link_libraries(FractalClient fractalInput ${LIBMFX})

    set_property(TARGET FractalClient PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Link to system libs
    target_link_libraries(FractalClient
            ${STATIC_OPENSSL}
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
            dxguid
            shcore
            DbgHelp
            )
endif()
