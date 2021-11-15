add_library(fractalUtils STATIC
        aes.h
        clock.h
        avpacket_buffer.h
        aes.c
        clock.c
        avpacket_buffer.c
        sysinfo.c
        png.c
        lodepng.c
        rwlock.c
        color.c
        threads.c
        )

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_sources(fractalUtils PRIVATE mac_utils.c mac_utils.h)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_sources(fractalUtils PRIVATE x11_window_info.c x11_window_info.h)
endif()

set_property(TARGET fractalUtils PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(fractalUtils ${STATIC_SDL2} ${CMAKE_DL_LIBS})
