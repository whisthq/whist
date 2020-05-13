add_library(fractalClipboard STATIC
        clipboard.h
        clipboard_synchronizer.h
        clipboard_synchronizer.c
        )

if (${UNIX} AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(fractalClipboard PRIVATE x11_clipboard.c)
elseif(${WIN32})
    target_sources(fractalClipboard PRIVATE win_clipboard.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(fractalClipboard PRIVATE mac_clipboard.c clipboard_osx.m)
endif ()