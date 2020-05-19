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
        set_property(TARGET fractalInput PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif ()
endif()