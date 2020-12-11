add_library(fractalNetwork STATIC
        network.c
        network.h)

target_link_libraries(fractalNetwork curl)

set_property(TARGET fractalNetwork PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
