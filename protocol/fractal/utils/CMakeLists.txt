add_library(fractalUtils STATIC
        aes.h
        clock.h
        logging.h
        sdlscreeninfo.h
        json.h
        aes.c
        clock.c
        logging.c
        sdlscreeninfo.c
        json.c
	sysinfo.c
        png.c
        mouse.c
        )

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(fractalUtils PRIVATE windows_utils.c windows_utils.h)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_sources(fractalUtils PRIVATE mac_utils.c mac_utils.h)
endif ()

set_property(TARGET fractalUtils PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(fractalUtils ${STATIC_SDL2} ${CMAKE_DL_LIBS})