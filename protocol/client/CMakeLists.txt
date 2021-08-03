#[[
################## FractalClient ##################
]]

add_executable(FractalClient
        main.c
        audio.c
        client_utils.c
        network.c
        sdl_event_handler.c
        sdl_utils.c
        handle_server_message.c
        video.c
        sdlscreeninfo.c
        peercursor.c
        ringbuffer.c
        sync.c
        )

# On Windows during client app packaging, FractalClient should have icon set
if(USE_CLIENT_APP_ICON AND ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(FractalClient
    PUBLIC executable_assets/WindowsAppIcon.rc
    )
endif()

# set the build directory
set_target_properties(FractalClient PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/client/build${arch}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/client/build${arch})

# copy the sentry shared library and crashpad handler to the build folder after the build
add_custom_command(
        TARGET FractalClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/lib/64/sentry/${CMAKE_SYSTEM_NAME}/
        $<TARGET_FILE_DIR:FractalClient>)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_custom_command(
            TARGET FractalClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/client/fclient
            ${CMAKE_BINARY_DIR})
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(
            TARGET FractalClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/client/fclient.bat
            ${CMAKE_BINARY_DIR})
endif()

# Copy over FFmpeg shared libs (on all OSes) into the FractalClient dir
add_custom_command(
	TARGET FractalClient POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_BINARY_DIR}/lib/64/ffmpeg/${CMAKE_SYSTEM_NAME}/
	$<TARGET_FILE_DIR:FractalClient>)

# OpenSSL/libcrypto --- on Windows, we need to copy the DLL into the right
# location to prevent a crash.
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(
            TARGET FractalClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/lib/64/openssl/Windows/libcrypto-1_1-x64.dll
            $<TARGET_FILE_DIR:FractalClient>)
endif()

# move necessary files into the build directory
add_custom_command(TARGET FractalClient PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/client/loading
        $<TARGET_FILE_DIR:FractalClient>/loading)

# Print out compile flags in verbose mode of CMake
message(VERBOSE "client C base flags: ${CMAKE_C_FLAGS}")
message(VERBOSE "client C release flags: ${CMAKE_C_FLAGS_RELEASE}")
message(VERBOSE "client C debug flags: ${CMAKE_C_FLAGS_DEBUG}")
message(VERBOSE "client C CI flags: ${CMAKE_C_FLAGS_CI}")

# FFmpeg, Threading and Dynamic linking libs
foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(VERBOSE "linking lib for FractalClient: ${LIB}")
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
        fractalVideo
        fractalClipboard
        fractalNetwork
        fractalLogging
        fractalUtils
        ${STATIC_SDL2}
        ${LIB_SENTRY}
        )

#[[
################## LINUX ##################
]]
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    target_sources(FractalClient PRIVATE native_window_utils_x11.c)

    find_package(X11 REQUIRED
            Xfixes)
    find_package(ALSA REQUIRED)
    include_directories(${X11_INCLUDE_DIR} ${ALSA_INCLUD_DIR})
    target_link_libraries(FractalClient
            ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            ${ALSA_LIBRARIES}
            crypto
            m
            )
endif()

#[[
################## MACOS ##################
]]
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(FractalClient PRIVATE native_window_utils_mac.m)

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
        ${LIB_OPENSSL}
        )
endif()

#[[
################## WINDOWS ##################
]]
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Linked flags: LNK4099 is when the linker cannot find the .pdb for debugging symbols
    # this comes up when we used precompiled libraries which we do not have .pdb files for
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099	")

    target_sources(FractalClient PRIVATE native_window_utils_windows.c)

    target_link_libraries(FractalClient fractalInput ${LIBMFX})

    set_property(TARGET FractalClient PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    if(CMAKE_CUDA_COMPILER)
        set_property(TARGET FractalClient PROPERTY CUDA_ARCHITECTURES OFF)
    endif()

    # Link to system libs
    target_link_libraries(FractalClient
            ${LIB_OPENSSL}
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
            setupapi
            )
endif()
