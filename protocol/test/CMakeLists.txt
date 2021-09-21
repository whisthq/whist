#[[
################## FractalUnitTests ##################
]]

# Create the executable
set(TEST_BINARY FractalUnitTests)
add_executable(${TEST_BINARY} main.cpp)

# Link the GoogleTest library, as well as the core client (but not main.c)
# TODO: also link server and other components of fractal
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
target_link_libraries(${TEST_BINARY} gtest FRACTAL_CLIENT_LIBRARY)

# Copy the Sentry library to the build folder
add_custom_command(
        TARGET ${TEST_BINARY} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/lib/64/sentry/${CMAKE_SYSTEM_NAME}/
        $<TARGET_FILE_DIR:${TEST_BINARY}>)
# Copy the FFmpeg library to the build folder
add_custom_command(
	TARGET ${TEST_BINARY} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_BINARY_DIR}/lib/64/ffmpeg/${CMAKE_SYSTEM_NAME}/
	$<TARGET_FILE_DIR:${TEST_BINARY}>)
# Copy the OpenSSL/libcrypto libraries if we are on Windows
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(
            TARGET ${TEST_BINARY} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/lib/64/openssl/Windows/libcrypto-1_1-x64.dll
            $<TARGET_FILE_DIR:${TEST_BINARY}>)
endif()

# Link the FFmpeg libraries
foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(STATUS "linking lib for ${TEST_BINARY}: ${LIB}")
    target_link_libraries(${TEST_BINARY} ${LIB})
endforeach(LIB)

# Link the threading and dynamic link library if we are on Mac/Linx
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(${TEST_BINARY} ${CMAKE_DL_LIBS})
endif()

# Link the remaining platform-independent libraries
target_link_libraries(${TEST_BINARY}
    ${PLATFORM_INDEPENDENT_LIBS}
)

# Link more linux-specific libraries
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    target_sources(${TEST_BINARY} PRIVATE ../client/native_window_utils_x11.c)

    find_package(X11 REQUIRED
            Xfixes)
    find_package(ALSA REQUIRED)
    include_directories(${X11_INCLUDE_DIR} ${ALSA_INCLUD_DIR})
    target_link_libraries(${TEST_BINARY} ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            ${ALSA_LIBRARIES}
            crypto
            m)
endif()

# Link Mac-specific libs
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(${TEST_BINARY} PRIVATE ../client/native_window_utils_mac.m)

    target_link_libraries(${TEST_BINARY} ${MAC_SPECIFIC_CLIENT_LIBS})
endif()

# Finally, link Windows-specific libraries
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099  ")

    target_sources(${TEST_BINARY} PRIVATE ../client/native_window_utils_windows.c)
    target_link_libraries(${TEST_BINARY} fractalInput ${LIBMFX})

    set_property(TARGET ${TEST_BINARY} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    if(CMAKE_CUDA_COMPILER)
        set_property(TARGET ${TEST_BINARY} PROPERTY CUDA_ARCHITECTURES OFF)
    endif()

    # Link to system libs
    target_link_libraries(${TEST_BINARY} ${WINDOWS_SPECIFIC_CLIENT_LIBS})
endif()

# Include the GoogleTest library, and let gtest scan for the tests in the binary file
include(GoogleTest)
gtest_discover_tests(${TEST_BINARY})
