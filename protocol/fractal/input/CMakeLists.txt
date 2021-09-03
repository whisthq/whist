if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_library(fractalInput STATIC
                input.c
                input.h
                input_driver.h
                keyboard_mapping.cpp
                keyboard_mapping.h
              )

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_sources(fractalInput PRIVATE
                xtest_input_driver.c
                xtest_input_driver.h
                uinput_input_driver.c
                uinput_input_driver.h)
    elseif(${WIN32})
        target_sources(fractalInput PRIVATE
            winapi_input_driver.c
            winapi_input_driver.h)
        set_property(TARGET fractalInput PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif ()
endif()
