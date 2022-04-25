set(GENERATED_SOURCES "")
set(WAYLAND_SCANNER "/home/david/install/whist/bin/wayland-scanner")

macro(generate_protocol_file PROTO)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/capture/${PROTO}-protocol.c"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/capture/"
		COMMAND ${WAYLAND_SCANNER} code < "${CMAKE_CURRENT_SOURCE_DIR}/capture/${PROTO}.xml" > "${CMAKE_CURRENT_BINARY_DIR}/capture/${PROTO}-protocol.c"
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/capture/${PROTO}.xml
    )

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/capture/${PROTO}-client-protocol.h"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/capture/"
		COMMAND ${WAYLAND_SCANNER} client-header < "${CMAKE_CURRENT_SOURCE_DIR}/capture/${PROTO}.xml" > "${CMAKE_CURRENT_BINARY_DIR}/capture/${PROTO}-client-protocol.h"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/capture/${PROTO}.xml"
    )

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/capture/${PROTO}-server-protocol.h"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/capture/
		COMMAND ${WAYLAND_SCANNER} server-header < ${CMAKE_CURRENT_SOURCE_DIR}/capture/${PROTO}.xml > ${CMAKE_CURRENT_BINARY_DIR}/capture/${PROTO}-server-protocol.h
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/capture/${PROTO}.xml
    )

    list(APPEND GENERATED_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/capture/${PROTO}-server-protocol.h)
    list(APPEND GENERATED_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/capture/${PROTO}-client-protocol.h)
    list(APPEND GENERATED_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/capture/${PROTO}-protocol.c)
endmacro()

add_library(whistVideo STATIC
        codec/decode.c
        video.c
        ltr.c
        capture/capture.h
        capture/capture.c
        capture/filecapture.c
        )

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(whistVideo PRIVATE
        transfercapture.c
        codec/encode.c
        codec/ffmpeg_encode.c
    )
    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    
    	generate_protocol_file(whist-protocol)
    
    	include_directories(/home/david/install/whist/include/ 
    		/home/david/install/whist/include/libweston-10 
    		/usr/include/pixman-1/ 
    		${CMAKE_CURRENT_BINARY_DIR}/capture/
    	)
    	target_link_libraries(whistVideo 
    		/home/david/install/whist/lib/x86_64-linux-gnu/libweston-10.so
    		/home/david/install/whist/lib/x86_64-linux-gnu/libweston-desktop-10.so
    		/home/david/install/whist/lib/x86_64-linux-gnu/libwayland-server.so
    		/home/david/install/whist/lib/x86_64-linux-gnu/libwayland-client.so
    		/home/david/install/whist/lib/x86_64-linux-gnu/weston/libexec_weston.so
    		/usr/lib/x86_64-linux-gnu/libcairo.so
    		xkbcommon
    	)
    
    	target_sources(whistVideo PRIVATE
    		capture/westoncommon.c
    	    capture/westoncapture.c
    	    capture/westoncompositor.c
    	    ${GENERATED_SOURCES}
    	)
    	
        target_sources(whistVideo PRIVATE
            capture/linuxcapture.c
            capture/x11capture.c
            #capture/nvidiacapture.c
            codec/nvidia_encode.c
            nvidia-linux/NvFBCUtils.c
            cudacontext.c
        )
    else()
        target_sources(whistVideo PRIVATE
            capture/windowscapture.c
        )
    endif()
endif()

set_property(TARGET whistVideo PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(whistVideo ${CMAKE_DL_LIBS})
