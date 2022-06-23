add_library(whistDebug STATIC
        plotter.cpp
        debug_console.cpp
        protocol_analyzer.cpp
        )

set_property(TARGET whistDebug PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
