if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_library(whistInput STATIC
        input.c
        input.h
        keyboard_mapping.cpp
        keyboard_mapping.h
    )

    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
      target_sources(whistInput PRIVATE xtest_input_driver.c
        uinput_input_driver.c
      )

    elseif(${WIN32})
        target_sources(whistInput PRIVATE winapi_input_driver.c)
        set_property(TARGET whistInput PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )
    endif()
endif()
