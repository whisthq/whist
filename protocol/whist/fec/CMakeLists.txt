add_subdirectory(cm256)
add_subdirectory(gf256)
add_subdirectory(wirehair)

add_library(whistFEC STATIC
        lugi_rs.c
        lugi_rs_extra.c
        rs_wrapper.c
        fec.c
        fec_controller.cpp
        )

target_link_libraries(whistFEC whistFEC_cm256 whistFEC_gf256 whistFEC_wirehair)

set_property(TARGET whistFEC PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
