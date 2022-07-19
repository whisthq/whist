# most of the libs are much slower with -O0
# e.g. cm256+gf256 is like 10 times slower
# it's better to compile as -O2 even for debug version

if(NOT MSVC)
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

add_subdirectory(cm256)
add_subdirectory(gf256)
add_subdirectory(wirehair)

add_library(whistFEC STATIC
        lugi_rs.c
        lugi_rs_extra.c
        rs_wrapper.c
        fec.c
        fec_controller.cpp
        wirehair_test.cpp
        )

target_link_libraries(whistFEC whistFEC_cm256 whistFEC_gf256 whistFEC_wirehair)

set_property(TARGET whistFEC PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
