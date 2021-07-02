add_library(fractalUtils STATIC
        aes.h
        clock.h
        avpacket_buffer.h
        logging.h
        error_monitor.h
        json.h
        window_name.h
        metrics.h
        aes.c
        clock.c
        avpacket_buffer.c
        logging.c
        error_monitor.c
        json.c
        window_name.c
        sysinfo.c
        png.c
        lodepng.c
        metrics.c
        mouse.c
        rwlock.c
        color.c
        threads.c
        )

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(fractalUtils PRIVATE windows_utils.c windows_utils.h)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_sources(fractalUtils PRIVATE mac_utils.c mac_utils.h)
endif()

set_property(TARGET fractalUtils PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(fractalUtils ${STATIC_SDL2} ${CMAKE_DL_LIBS})
