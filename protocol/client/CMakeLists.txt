#[[
################## WhistClient ##################
]]

set(WHIST_CLIENT_DEPENDS
        whist_client.c
        audio.c
        client_utils.c
        network.c
        sdl_event_handler.c
        sdl_utils.c
        handle_server_message.c
        video.c
        sdlscreeninfo.c
        sync_packets.c
        client_statistic.c
        renderer.c
)

if(CLIENT_SHARED_LIB)
  add_library(WhistClient SHARED
    ${WHIST_CLIENT_DEPENDS}
  )
  add_custom_command(
    TARGET WhistClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/client/whist_client.h
    $<TARGET_FILE_DIR:WhistClient>/WhistClient.h
  )
else()
  add_executable(WhistClient
    main.c
    ${WHIST_CLIENT_DEPENDS}
  )
endif()

# On Windows during client app packaging, WhistClient should have icon set
if(USE_CLIENT_APP_ICON AND ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(WhistClient
    PUBLIC executable_assets/WindowsAppIcon.rc
    )
endif()

# set the build directory
set_target_properties(WhistClient PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/client/build${arch}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/client/build${arch})

# copy the Sentry shared library and crashpad handler to the build folder after the build
add_custom_command(
        TARGET WhistClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/lib/64/sentry/${CMAKE_SYSTEM_NAME}/
        $<TARGET_FILE_DIR:WhistClient>)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_custom_command(
            TARGET WhistClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/client/wclient
            ${CMAKE_BINARY_DIR})

endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(
            TARGET WhistClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/client/wclient.bat
            ${CMAKE_BINARY_DIR})
endif()

# Copy over FFmpeg shared libs (on all OSes) into the WhistClient dir
add_custom_command(
    TARGET WhistClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/lib/64/ffmpeg/${CMAKE_SYSTEM_NAME}/
    $<TARGET_FILE_DIR:WhistClient>)

# OpenSSL/libcrypto --- on Windows, we need to copy the DLL into the right
# location to prevent a crash.
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(
            TARGET WhistClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/lib/64/openssl/Windows/libcrypto-1_1-x64.dll
            $<TARGET_FILE_DIR:WhistClient>)
endif()

# move necessary files into the build directory
add_custom_command(TARGET WhistClient PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/client/images
        $<TARGET_FILE_DIR:WhistClient>/images)

add_custom_command(TARGET WhistClient PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/client/images
        $<TARGET_FILE_DIR:WhistClient>/images)

# Print out compile flags in verbose mode of CMake
message(VERBOSE "client C base flags: ${CMAKE_C_FLAGS}")
message(VERBOSE "client C release flags: ${CMAKE_C_FLAGS_RELEASE}")
message(VERBOSE "client C debug flags: ${CMAKE_C_FLAGS_DEBUG}")
message(VERBOSE "client C CI flags: ${CMAKE_C_FLAGS_CI}")

# FFmpeg, Threading and Dynamic linking libs
foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(VERBOSE "linking lib for WhistClient: ${LIB}")
    target_link_libraries(WhistClient ${LIB})
endforeach()

# Threading and Dynamic link for linux/macosx only
# if we add other target platforms we may need to come back to this
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#    set(THREADS_PREFER_PTHREAD_FLAG ON)
#    find_package(Threads REQUIRED)
    target_link_libraries(WhistClient
#            Threads::Threads
            ${CMAKE_DL_LIBS}
            )
endif()

#[[
################## Linking platform independent libs ##################
]]
target_link_libraries(WhistClient ${PLATFORM_INDEPENDENT_LIBS})

#[[
################## LINUX ##################
]]
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(WhistClient PRIVATE
        native_window_utils_x11.c
    )

    find_package(X11 REQUIRED
            Xfixes)
    find_package(ALSA REQUIRED)
    include_directories(${X11_INCLUDE_DIR} ${ALSA_INCLUD_DIR})
    target_link_libraries(WhistClient
            ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            ${ALSA_LIBRARIES}
            crypto
            m)
endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND ${CMAKE_BUILD_TYPE} MATCHES "Release")
    add_custom_command(
            TARGET WhistClient POST_BUILD
            COMMAND objcopy --only-keep-debug $<TARGET_FILE:WhistClient> ${CMAKE_BINARY_DIR}/client/build${arch}/WhistClient.debug
            COMMAND objcopy --strip-debug --strip-unneeded $<TARGET_FILE:WhistClient>)
endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND ${CMAKE_BUILD_TYPE} MATCHES "Release")
    add_custom_command(
            TARGET WhistClient POST_BUILD
            COMMAND /usr/bin/dsymutil $<TARGET_FILE:WhistClient> -o ${CMAKE_BINARY_DIR}/client/build${arch}/WhistClient.debug
            COMMAND /usr/bin/strip -S $<TARGET_FILE:WhistClient>)
endif()


#[[
################## MACOS ##################
]]
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(WhistClient PRIVATE
        native_window_utils_mac.m
        notif_utils/display_notifs_mac.m
    )

    target_link_libraries(WhistClient ${MAC_SPECIFIC_CLIENT_LIBS})
endif()

#[[
################## WINDOWS ##################
]]
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Linked flags: LNK4099 is when the linker cannot find the .pdb for debugging symbols
    # this comes up when we used precompiled libraries which we do not have .pdb files for
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099 ")

    target_sources(WhistClient PRIVATE
        native_window_utils_windows.c
    )

    target_link_libraries(WhistClient whistInput ${LIBMFX})

    set_property(TARGET WhistClient PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    if(CMAKE_CUDA_COMPILER)
        set_property(TARGET WhistClient PROPERTY CUDA_ARCHITECTURES OFF)
    endif()

    # Link to system libs
    target_link_libraries(WhistClient ${WINDOWS_SPECIFIC_CLIENT_LIBS})
endif()
