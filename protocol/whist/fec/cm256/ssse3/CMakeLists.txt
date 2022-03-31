if(MSVC)
    #message(WARNING "cm256_ssse3: Current compiler is MSVC!!!")
    # the MSVC on ci doesn't recogized the below option
    # but it does exist on some document
    # we simply keep this option, since there is no harm to do
    add_compile_options("/arch:SSSE3")
else()
    #message(WARNING "cm256_ssse3: Current compiler is not MSVC!!!")
    add_compile_options("-mssse3")
endif()

add_library(whistFEC_cm256_ssse3 STATIC gf256_ssse3.cpp)
