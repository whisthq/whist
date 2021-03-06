add_library(fractalCore STATIC
        fractal.h
        fractal.c
        fractalgetopt.h
        fractalgetopt.c)

set_property(TARGET fractalCore PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")