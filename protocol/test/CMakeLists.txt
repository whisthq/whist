# #[[
################## FractalProtocolUnitTests ##################
#]]

    # Create the executable
    set(PROTOCOL_TEST_BINARY FractalProtocolTest)
    
    # Server source files do not compile on Mac
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        add_executable(${PROTOCOL_TEST_BINARY} ProtocolTest.cpp

            # Files needed for Fractal Lib unit tests 
            ../fractal/utils/aes.c
            ../fractal/utils/png.c
            ../fractal/utils/avpacket_buffer.c

            # Files needed for Client unit tests
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
            ../client/bitrate.c
        )
    endif()
    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        add_executable(${PROTOCOL_TEST_BINARY} ProtocolTest.cpp
        # Files needed for Server unit tests
        ../server/client.c
        ../server/video.c
        ../server/audio.c
        ../server/parse_args.c
        ../server/handle_client_message.c
        ../server/network.c

        # Files needed for Fractal Lib unit tests 
        ../fractal/utils/aes.c
        ../fractal/utils/png.c
        ../fractal/utils/avpacket_buffer.c

        # Files needed for Client unit tests
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
        ../client/bitrate.c
    )
    endif()


    # Link the GoogleTest library
    target_link_libraries(${PROTOCOL_TEST_BINARY} gtest)

    # Copy the Sentry library to the build folder
    add_custom_command(
            TARGET ${PROTOCOL_TEST_BINARY} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_BINARY_DIR}/lib/64/sentry/${CMAKE_SYSTEM_NAME}/
            $<TARGET_FILE_DIR:${PROTOCOL_TEST_BINARY}>)

    # Copy the FFmpeg library to the build folder
    add_custom_command(
        TARGET ${PROTOCOL_TEST_BINARY} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/lib/64/ffmpeg/${CMAKE_SYSTEM_NAME}/
        $<TARGET_FILE_DIR:${PROTOCOL_TEST_BINARY}>)

    # Copy the OpenSSL/libcrypto libraries if we are on Windows
    if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        add_custom_command(
                TARGET ${PROTOCOL_TEST_BINARY} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/lib/64/openssl/Windows/libcrypto-1_1-x64.dll
                $<TARGET_FILE_DIR:${PROTOCOL_TEST_BINARY}>)
    endif()

    # Link the FFmpeg libraries 
    foreach(LIB ${FFMPEG_LIBS_PATHS})
        message(VERBOSE "linking lib for ${PROTOCOL_TEST_BINARY}: ${LIB}")
        target_link_libraries(${PROTOCOL_TEST_BINARY} ${LIB})
    endforeach(LIB)

    # Link the threading and dynamic link library if we are on Mac/Linx
    if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        target_link_libraries(${PROTOCOL_TEST_BINARY} ${CMAKE_DL_LIBS})
    endif()

    # Link the remaining platform-independent libraries
    target_link_libraries(${PROTOCOL_TEST_BINARY}
        ${PLATFORM_INDEPENDENT_SERVER_LIBS}
        ${PLATFORM_INDEPENDENT_LIBS}
    )
    
    # Link macOS-specific libs
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_sources(${PROTOCOL_TEST_BINARY} PRIVATE
            ../client/native_window_utils_mac.m
        )

        target_link_libraries(${PROTOCOL_TEST_BINARY} ${MAC_SPECIFIC_CLIENT_LIBS})
    endif()

    # Link more linux-specific libraries    
    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

        target_sources(${PROTOCOL_TEST_BINARY} PRIVATE ../client/native_window_utils_x11.c)
        find_package(X11 REQUIRED
                Xfixes
                Xdamage
                Xext
                Xtst)
        find_package(ALSA REQUIRED)
        include_directories(${X11_INCLUDE_DIR} ${ALSA_INCLUD_DIR})
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

        target_link_libraries(${PROTOCOL_TEST_BINARY}
                ${X11_LIBRARIES}
                ${X11_Xfixes_LIB}
                ${X11_Xdamage_LIB}
                ${X11_Xext_LIB}
                ${X11_Xtst_LIB}
                ${ALSA_LIBRARIES}
                GL
                asound
                m
                OpenSSL::Crypto
                )
    endif()


    add_custom_command(
        TARGET ${PROTOCOL_TEST_BINARY} POST_BUILD
        COMMAND ./${PROTOCOL_TEST_BINARY}
    )

    if (GENERATE_COVERAGE)
        add_custom_command(
            TARGET ${PROTOCOL_TEST_BINARY} POST_BUILD
            COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR} --output-file ${CMAKE_BINARY_DIR}/coverage.info
            COMMAND lcov --remove ${CMAKE_BINARY_DIR}/coverage.info -o ${CMAKE_BINARY_DIR}/coverage.info '${CMAKE_BINARY_DIR}/*' '/Applications/*' '/usr/include/c++/9/*' '/Library/*' '${CMAKE_SOURCE_DIR}/test' # Remove non-Fractal files
            COMMAND lcov --list ${CMAKE_BINARY_DIR}/coverage.info #  print verbose coverage information
        )
    endif()

    if (UPLOAD_COVERAGE)
        add_custom_command(
            TARGET ${PROTOCOL_TEST_BINARY} POST_BUILD
            COMMAND ${CMAKE_BINARY_DIR}/codecov --sha $ENV{GITHUB_SHA} --slug fractal/fractal -t $ENV{CODECOV_TOKEN} -c -F protocol
        )
    endif()

#[[
################## FractalUnitTests ##################
]]

# Include the GoogleTest library, and let gtest scan for the tests in the binary file
include(GoogleTest)
gtest_discover_tests(${PROTOCOL_TEST_BINARY})
