add_library(fractalCore STATIC
        fractal.c
        fractal.h)

set_property(TARGET fractalCore PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")