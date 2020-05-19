add_library(fractalVideo STATIC
        screencapture.h
        videodecode.c
        videodecode.h
        videoencode.c
        videoencode.h
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(fractalVideo PRIVATE x11capture.c x11capture.h)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(fractalVideo PRIVATE dxgicapture.c dxgicapture.h)
endif ()

set_property(TARGET fractalVideo PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(fractalVideo ${STATIC_SDL2} ${CMAKE_DL_LIBS})
