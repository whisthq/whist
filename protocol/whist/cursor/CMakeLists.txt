if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_library(whistCursor STATIC linuxcursor.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_library(whistCursor STATIC windowscursor.c)
    set_property(TARGET whistCursor PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
