add_library(fractalCursor STATIC
        peercursor.h
        peercursor.c
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(fractalCursor PRIVATE linuxcursor.c cursor.h)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(fractalCursor PRIVATE windowscursor.c cursor.h)
    set_property(TARGET fractalCursor PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()
