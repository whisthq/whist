add_library(whistFile STATIC
        file_drop.c
        file_synchronizer.c
        )

set_property(TARGET whistFile PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
