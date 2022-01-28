add_library(whistClipboard STATIC
        clipboard.h
        clipboard_synchronizer.h
        clipboard_synchronizer.c
        clipboard.c
        )

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(whistClipboard PRIVATE x11_clipboard.c)
elseif(${WIN32})
    target_sources(whistClipboard PRIVATE win_clipboard.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(whistClipboard PRIVATE mac_clipboard.c clipboard_osx.m)
endif()

set_property(TARGET whistClipboard PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
