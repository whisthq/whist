add_library(whistFEC STATIC
        lugi_rs.c
        fec.c
        )

set_property(TARGET whistFEC PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
