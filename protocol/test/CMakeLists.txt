#[[
################## FractalClientUnitTests ##################
]]

# Create the executable
set(CLIENT_TEST_BINARY FractalClientTest)
add_executable(${CLIENT_TEST_BINARY} ClientTest.cpp)

# Link the GoogleTest library, as well as the core client (but not main.c)
set(FRACTAL_CLIENT_SOURCEFILES
        ../client/audio.c
        ../client/client_utils.c
        ../client/network.c
        ../client/sdl_event_handler.c
        ../client/sdl_utils.c
        ../client/handle_server_message.c
        ../client/video.c
        ../client/sdlscreeninfo.c
        ../client/peercursor.c
        ../client/ringbuffer.c
        ../client/sync_packets.c
        ../client/bitrate.c)
add_library(FRACTAL_CLIENT_LIBRARY ${FRACTAL_CLIENT_SOURCEFILES})
target_link_libraries(${CLIENT_TEST_BINARY} gtest FRACTAL_CLIENT_LIBRARY)

# Copy the Sentry library to the build folder
add_custom_command(
        TARGET ${CLIENT_TEST_BINARY} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/lib/64/sentry/${CMAKE_SYSTEM_NAME}/
        $<TARGET_FILE_DIR:${CLIENT_TEST_BINARY}>)
# Copy the FFmpeg library to the build folder
add_custom_command(
	TARGET ${CLIENT_TEST_BINARY} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_BINARY_DIR}/lib/64/ffmpeg/${CMAKE_SYSTEM_NAME}/
	$<TARGET_FILE_DIR:${CLIENT_TEST_BINARY}>)
# Copy the OpenSSL/libcrypto libraries if we are on Windows
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(
            TARGET ${CLIENT_TEST_BINARY} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/lib/64/openssl/Windows/libcrypto-1_1-x64.dll
            $<TARGET_FILE_DIR:${CLIENT_TEST_BINARY}>)
endif()

# Link the FFmpeg libraries
foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(VERBOSE "linking lib for ${CLIENT_TEST_BINARY}: ${LIB}")
    target_link_libraries(${CLIENT_TEST_BINARY} ${LIB})
endforeach(LIB)

# Link the threading and dynamic link library if we are on Mac/Linx
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(${CLIENT_TEST_BINARY} ${CMAKE_DL_LIBS})
endif()

# Link the remaining platform-independent libraries
target_link_libraries(${CLIENT_TEST_BINARY}
    ${PLATFORM_INDEPENDENT_LIBS}
)

# Link more linux-specific libraries
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    target_sources(${CLIENT_TEST_BINARY} PRIVATE ../client/native_window_utils_x11.c)

    find_package(X11 REQUIRED
            Xfixes)
    find_package(ALSA REQUIRED)
    include_directories(${X11_INCLUDE_DIR} ${ALSA_INCLUD_DIR})
    target_link_libraries(${CLIENT_TEST_BINARY} ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            ${ALSA_LIBRARIES}
            crypto
            m)
endif()

# Link Mac-specific libs
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(${CLIENT_TEST_BINARY} PRIVATE ../client/native_window_utils_mac.m)

    target_link_libraries(${CLIENT_TEST_BINARY} ${MAC_SPECIFIC_CLIENT_LIBS})
endif()

# Finally, link Windows-specific libraries
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099  ")

    target_sources(${CLIENT_TEST_BINARY} PRIVATE ../client/native_window_utils_windows.c)
    target_link_libraries(${CLIENT_TEST_BINARY} fractalInput ${LIBMFX})

    set_property(TARGET ${CLIENT_TEST_BINARY} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    if(CMAKE_CUDA_COMPILER)
        set_property(TARGET ${CLIENT_TEST_BINARY} PROPERTY CUDA_ARCHITECTURES OFF)
    endif()

    # Link to system libs
    target_link_libraries(${CLIENT_TEST_BINARY} ${WINDOWS_SPECIFIC_CLIENT_LIBS})
endif()


#[[
################## FractalServerUnitTests ##################
]]

# Create the executable
set(SERVER_TEST_BINARY FractalServerTest)
add_executable(${SERVER_TEST_BINARY} ServerTest.cpp)

# Link the GoogleTest library, as well as the core client (but not main.c)
set(FRACTAL_SERVER_SOURCEFILES
        ../server/client.c
        ../server/video.c
        ../server/audio.c
        ../server/parse_args.c
        ../server/handle_client_message.c
        ../server/network.c)
add_library(FRACTAL_SERVER_LIBRARY ${FRACTAL_SERVER_SOURCEFILES})
target_link_libraries(${SERVER_TEST_BINARY} gtest FRACTAL_SERVER_LIBRARY)

# Link the FFmpeg libraries
foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(VERBOSE "linking lib for ${SERVER_TEST_BINARY}: ${LIB}")
    target_link_libraries(${SERVER_TEST_BINARY} ${LIB})
endforeach(LIB)

# Link the threading and dynamic link library if we are on Mac/Linx
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(${SERVER_TEST_BINARY} ${CMAKE_DL_LIBS})
endif()

# Link the remaining platform-independent libraries
target_link_libraries(${SERVER_TEST_BINARY}
    ${PLATFORM_INDEPENDENT_SERVER_LIBS}
)

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

    target_link_libraries(${SERVER_TEST_BINARY}
            ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            ${X11_Xdamage_LIB}
            ${X11_Xext_LIB}
            ${X11_Xtst_LIB}
            GL
            asound
            m
            OpenSSL::Crypto
            )
endif()


if (${WIN32})
    #Linked flags: LNK4099 is when the linker cannot find the .pdb for debugging symbols
    # this comes up when we used precompiled libraries which we do not have .pdb files for
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099  ")

    #sets the MSVC runtime library as multithreaded and adds debug symbols if is a debug build
    #see https://cmake.org/cmake/help/v3.15/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html#variable:CMAKE_MSVC_RUNTIME_LIBRARY
    set_property(TARGET ${SERVER_TEST_BINARY} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    if(CMAKE_CUDA_COMPILER)
        set_property(TARGET ${SERVER_TEST_BINARY} PROPERTY CUDA_ARCHITECTURES OFF)
    endif()

    # Link to system libs
    target_link_libraries(${SERVER_TEST_BINARY} ${WINDOWS_SPECIFIC_SERVER_LIBS})
endif()


#[[
################## FractalUnitTests ##################
]]


# Include the GoogleTest library, and let gtest scan for the tests in the binary file
include(GoogleTest)
gtest_discover_tests(${CLIENT_TEST_BINARY})
gtest_discover_tests(${SERVER_TEST_BINARY})
