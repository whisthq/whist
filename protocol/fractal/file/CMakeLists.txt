add_library(fractalFile STATIC
        file_synchronizer.h
        file_synchronizer.c
        )

set_property(TARGET fractalFile PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
