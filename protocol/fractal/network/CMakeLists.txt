add_library(fractalNetwork STATIC
        network.c
        network.h)

set_property(TARGET fractalNetwork PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")