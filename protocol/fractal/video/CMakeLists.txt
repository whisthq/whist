add_library(fractalVideo STATIC
        screencapture.h
        videodecode.c
        videodecode.h
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(fractalVideo PRIVATE x11capture.c x11capture.h)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(fractalVideo PRIVATE dxgicapture.c dxgicapture.h)
    target_sources(fractalVideo PRIVATE dxgicudabridge.cu dxgicudabridge.h)
endif ()

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(fractalVideo PRIVATE videoencode.c videoencode.h)
endif ()




set_property(TARGET fractalVideo PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(fractalVideo ${STATIC_SDL2} ${CMAKE_DL_LIBS})
