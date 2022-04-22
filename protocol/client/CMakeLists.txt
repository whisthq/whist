#[[
################## WhistClient ##################
]]

set(WHIST_CLIENT_DEPENDS
        whist_client.c
        audio.c
        client_utils.c
        network.c
        handle_frontend_events.c
        sdl_utils.c
        handle_server_message.c
        video.c
        sync_packets.c
        renderer.c
        frontend/frontend.c
        frontend/sdl/audio.c
        frontend/sdl/events.c
        frontend/sdl/lifecycle.c
        frontend/sdl/sdl.c
        frontend/sdl/video.c
        frontend/sdl/window.c
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

copy_runtime_libs(WhistClient)

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

# move necessary files into the build directory
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
        frontend/sdl/native_x11.c
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


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_custom_command(
            TARGET WhistClient POST_BUILD
            COMMAND "$<IF:$<CONFIG:Release>,objcopy,true>" --only-keep-debug "$<TARGET_FILE:WhistClient>" "${CMAKE_BINARY_DIR}/client/build${arch}/WhistClient.debug"
            COMMAND "$<IF:$<CONFIG:Release>,objcopy,true>" --strip-debug --strip-unneeded "$<TARGET_FILE:WhistClient>"
            COMMAND_EXPAND_LISTS
    )
endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_custom_command(
            TARGET WhistClient POST_BUILD
            COMMAND "$<IF:$<CONFIG:Release>,/usr/bin/dsymutil,true>" "$<TARGET_FILE:WhistClient>" -o "${CMAKE_BINARY_DIR}/client/build${arch}/WhistClient.debug"
            COMMAND "$<IF:$<CONFIG:Release>,/usr/bin/strip,true>" -S $<TARGET_FILE:WhistClient>
            COMMAND_EXPAND_LISTS
    )
endif()


#[[
################## MACOS ##################
]]
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(WhistClient PRIVATE
        frontend/sdl/native_macos.m
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
        frontend/sdl/native_windows.c
    )

    set_property(TARGET WhistClient PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    if(CMAKE_CUDA_COMPILER)
        set_property(TARGET WhistClient PROPERTY CUDA_ARCHITECTURES OFF)
    endif()

    # Link to system libs
    target_link_libraries(WhistClient ${WINDOWS_SPECIFIC_CLIENT_LIBS})
endif()
