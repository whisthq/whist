if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_library(fractalCursor STATIC linuxcursor.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_library(fractalCursor STATIC windowscursor.c)
    set_property(TARGET fractalCursor PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
