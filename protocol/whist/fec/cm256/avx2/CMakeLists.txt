if(MSVC)
    #message(WARNING "Current compiler is MSVC!!!")
    add_compile_options("/arch:AVX2")
else()
    #message(WARNING "Current compiler is not MSVC!!!")
    add_compile_options("-mavx2")
endif()

if(NOT MSVC)
        #message(WARNING "Current compiler is NOT MSVC!!!")
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

add_library(whistFEC_cm256_avx2 STATIC gf256_avx2.cpp)
