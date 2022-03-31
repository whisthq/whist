if(MSVC)
    #message(WARNING "Current compiler is MSVC!!!")
    add_compile_options("/arch:SSSE3")
else()
    #message(WARNING "Current compiler is not MSVC!!!")
    add_compile_options("-mssse3")
endif()

add_library(whistFEC_cm256_ssse3 STATIC gf256_ssse3.cpp)
