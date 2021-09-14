set(BINARY FractalClient_test)

#file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp)

#set(SOURCES ${TEST_SOURCES})
enable_testing()

# set(FRACTAL_CLIENT_SOURCEFILES #../client/main.c
#         ../client/audio.c
#         ../client/client_utils.c
#         ../client/network.c
#         ../client/sdl_event_handler.c
#         ../client/sdl_utils.c
#         ../client/handle_server_message.c
#         ../client/video.c
#         ../client/sdlscreeninfo.c
#         ../client/peercursor.c
#         ../client/ringbuffer.c
#         ../client/sync_packets.c
#         ../client/bitrate.c)

#add_executable(${BINARY} main.cpp ${FRACTAL_CLIENT_SOURCEFILES})

add_executable(${BINARY} main.cpp ../client/main.c
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

add_test(NAME ${BINARY} COMMAND ${BINARY})

#target_link_libraries(${BINARY} PUBLIC FRACTAL_CLIENT_LIBRARY gtest)

target_link_libraries(${BINARY} gtest)

# FFmpeg, Threading and Dynamic linking libs
foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(STATUS "linking lib for ${BINARY}: ${LIB}")
    target_link_libraries(${BINARY} ${LIB})
endforeach(LIB)

# Threading and Dynamic link for linux/macosx only
# if we add other target platforms we may need to come back to this
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#    set(THREADS_PREFER_PTHREAD_FLAG ON)
#    find_package(Threads REQUIRED)
    target_link_libraries(${BINARY}
#            Threads::Threads
            ${CMAKE_DL_LIBS}
            )
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


include(GoogleTest)
gtest_discover_tests(${BINARY})