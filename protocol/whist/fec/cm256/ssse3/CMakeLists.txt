if(NOT MSVC)
    # message(WARNING "cm256_ssse3: Current compiler is not MSVC!!!")
    # there is no similiar option in MSVC, and MSVC enables ssse3 by default
    add_compile_options("-mssse3")
endif()

add_library(whistFEC_cm256_ssse3 STATIC gf256_ssse3.cpp)
