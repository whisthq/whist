set(BINARY FractalClient_test)

enable_testing()

add_executable(${BINARY} main.cpp)

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

target_link_libraries(${BINARY} gtest FRACTAL_CLIENT_LIBRARY)

add_custom_command(
        TARGET ${BINARY} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/lib/64/sentry/${CMAKE_SYSTEM_NAME}/
        $<TARGET_FILE_DIR:${BINARY}>)

add_custom_command(
	TARGET ${BINARY} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_BINARY_DIR}/lib/64/ffmpeg/${CMAKE_SYSTEM_NAME}/
	$<TARGET_FILE_DIR:${BINARY}>)


# FFmpeg, Threading and Dynamic linking libs
foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(STATUS "linking lib for ${BINARY}: ${LIB}")
    target_link_libraries(${BINARY} ${LIB})
endforeach(LIB)


if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(${BINARY} ${CMAKE_DL_LIBS})
endif()

target_link_libraries(${BINARY}
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

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(${BINARY} PRIVATE ../client/native_window_utils_mac.m)

    target_link_libraries(${BINARY}
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

add_test(NAME ${BINARY} COMMAND ${BINARY})


include(GoogleTest)
gtest_discover_tests(${BINARY})