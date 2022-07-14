# cm256+gf256 is like 10 times slower when compiled with -O0
# it's better to compile as -O2 even for debug version

if(NOT MSVC)
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

add_library(whistFEC_cm256 STATIC
        cm256.cpp
        )

