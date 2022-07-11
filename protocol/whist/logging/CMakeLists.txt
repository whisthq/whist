add_library(whistLogging STATIC
        logging.h
        log_statistic.h
        error_monitor.h
        logging.c
        log_statistic.c
        error_monitor.c
        )

set_property(TARGET whistLogging PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(whistLogging whistCore ${CMAKE_DL_LIBS})
