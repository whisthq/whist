if(MSVC)
    #message(WARNING "Current compiler is MSVC!!!")
    add_compile_options("/arch:SSSE3")
else()
    #message(WARNING "Current compiler is not MSVC!!!")
    add_compile_options("-mssse3")
endif()

if(NOT MSVC)
        #message(WARNING "Current compiler is NOT MSVC!!!")
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

add_library(whistFEC_cm256_ssse3 STATIC gf256_ssse3.cpp)
