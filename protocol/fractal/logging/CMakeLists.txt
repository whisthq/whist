add_library(fractalLogging STATIC
        logging.h
        log_statistic.h
        error_monitor.h
        logging.c
        log_statistic.c
        error_monitor.c
        )

set_property(TARGET fractalLogging PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(fractalLogging ${STATIC_SDL2} ${CMAKE_DL_LIBS})
