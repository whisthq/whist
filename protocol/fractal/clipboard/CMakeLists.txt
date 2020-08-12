add_library(fractalClipboard STATIC
        clipboard.h
        clipboard_synchronizer.h
        clipboard_synchronizer.c
        clipboard.c
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(fractalClipboard PRIVATE x11_clipboard.c)
elseif(${WIN32})
    target_sources(fractalClipboard PRIVATE win_clipboard.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(fractalClipboard PRIVATE mac_clipboard.c clipboard_osx.m)
endif ()

set_property(TARGET fractalClipboard PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")