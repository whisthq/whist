if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_library(fractalInput "")

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
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
endif()