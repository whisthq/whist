add_library(fractalVideo STATIC
        screencapture.h
        videodecode.h
        videodecode.h
        videodecode.c
        videoencode.h
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(fractalVideo PRIVATE x11capture.c x11capture.h)
elseif(${WIN32})
    target_sources(fractalVideo PRIVATE dxgicapture.c dxgicapture.h)
endif ()

set_property(TARGET fractalVideo PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")