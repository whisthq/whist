add_library(whistCursor STATIC cursor.c cursor_internal.c)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(whistCursor PRIVATE linux_cursor_capture.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(whistCursor PRIVATE windows_cursor_capture.c)
    set_property(TARGET whistCursor PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
