# this lib is like 10 times slower when compiled with -O0
# it's better to compile as -O2 even for debug version

if(NOT MSVC)
        #message(WARNING "gf256: Current compiler is NOT MSVC!!!")
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

#message(WARNING "gf256: Build type is:" ${CMAKE_BUILD_TYPE} "!!!")

# the below detection works since the only arm we support is M1,
# otherwise much more complex detections are needed
if((${CMAKE_SYSTEM_NAME} MATCHES "Darwin") AND (${MACOS_ARCHITECTURE} MATCHES "arm"))
        #message(WARNING "cm256: Current ARCH is macos arm!!!")
        set(GF256_IS_ARM TRUE)
else()
        set(GF256_IS_ARM FALSE)
endif()

add_library(whistFEC_gf256 STATIC
        gf256.cpp
        )

if(NOT ${GF256_IS_ARM})
        add_subdirectory(avx2)
        add_subdirectory(ssse3)
        target_link_libraries(whistFEC_gf256 whistFEC_gf256_avx2)
        target_link_libraries(whistFEC_gf256 whistFEC_gf256_ssse3)
endif()
