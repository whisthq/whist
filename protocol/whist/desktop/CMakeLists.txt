add_library(whistDesktop STATIC
        desktop.c
        desktop_x11.c
        )

set_property(TARGET whistDesktop PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
