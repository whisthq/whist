if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        add_library(fractalCursor STATIC
                cursor.h
                linuxcursor.c
                )
    elseif((${CMAKE_SYSTEM_NAME} MATCHES "Windows"))
        add_library(fractalCursor STATIC
                cursor.h
                windowscursor.c
                )
    endif()
endif()