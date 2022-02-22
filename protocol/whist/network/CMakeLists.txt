add_library(whistNetwork STATIC
        network.c
        udp.c
        tcp.c
        throttle.c
        ringbuffer.c
        network_algorithm.c
        socket.c
        socket_bsd.c
    )

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(whistNetwork curl)
else()
    target_link_libraries(whistNetwork winhttp)
endif()

set_property(TARGET whistNetwork PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
