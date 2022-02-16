add_library(whistCursor STATIC cursor.c)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(whistCursor PRIVATE linuxcursor.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(whistCursor PRIVATE windowscursor.c)
    set_property(TARGET whistCursor PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
