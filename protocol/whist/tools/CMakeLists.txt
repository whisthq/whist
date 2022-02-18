add_library(whistTools STATIC
        debug_console.cpp
        protocol_analyzer.cpp
        )

set_property(TARGET whistTools PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
