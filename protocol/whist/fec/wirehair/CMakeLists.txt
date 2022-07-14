# this lib is much slower when compiled with -O0
# it's better to compile as -O2 even for debug version

if(NOT MSVC)
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

add_library(whistFEC_wirehair STATIC
        WirehairCodec.cpp
        WirehairTools.cpp
        wirehair.cpp
        wirehair_test.cpp
        )

