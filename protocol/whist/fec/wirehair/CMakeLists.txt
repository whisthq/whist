# this lib is like 10 times slower when compiled with -O0
# it's better to compile as -O2 even for debug version

if(NOT MSVC)
        #message(WARNING "cm256: Current compiler is NOT MSVC!!!")
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

#message(WARNING "cm256: Build type is:" ${CMAKE_BUILD_TYPE} "!!!")

add_library(whistFEC_wirehair STATIC
        WirehairCodec.cpp
        WirehairTools.cpp
        wirehair.cpp
        wirehair_test.cpp
        )


