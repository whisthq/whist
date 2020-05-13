if (${UNIX} AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_library(fractalCursor STATIC
            cursor.h
            linuxcursor.c
            )
elseif(${WIN32})
    add_library(fractalCursor STATIC
            cursor.h
            windowscursor.c.c
            )
endif()