add_library(whistNetwork STATIC
        network.c
        udp.cpp
        tcp.c
        throttle.c
        ringbuffer.c
        network_algorithm.c
    )

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(whistNetwork curl)
else()
    target_link_libraries(whistNetwork winhttp)
endif()

set_property(TARGET whistNetwork PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
