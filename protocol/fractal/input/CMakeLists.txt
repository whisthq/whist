add_library(fractalInput "")

if (${UNIX} AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(fractalInput PRIVATE
            input.h
            linuxinput.c
            linuxinput.h)
elseif(${WIN32})
    target_sources(fractalInput PRIVATE
        input.h
        windowsinput.c
        windowsinput.h)
endif ()