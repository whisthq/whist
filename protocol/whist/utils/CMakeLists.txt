add_library(whistUtils STATIC
        aes.h
        clock.h
        avpacket_buffer.h
        window_info.h
        aes.c
        clock.c
        avpacket_buffer.c
        sysinfo.c
        png.c
        lodepng.c
        rwlock.c
        color.c
        threads.c
        fec.c
        rs.c
        os_utils.c
        linked_list.c
        queue.c
	audio_queue.cpp
        )

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_sources(whistUtils PRIVATE mac_utils.c mac_utils.h ffmpeg_memory.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_sources(whistUtils PRIVATE x11_window_info.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        target_sources(whistUtils PRIVATE windows_window_info.c)
endif()

set_property(TARGET whistUtils PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(whistUtils ${STATIC_SDL2} ${CMAKE_DL_LIBS})
