# this lib is like 10 times slower when compiled with -O0
# it's better to compile as -O2 even for debug version

if(NOT MSVC)
        #message(WARNING "Current compiler is NOT MSVC!!!")
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

message(WARNING "Build type is:" ${CMAKE_BUILD_TYPE} "!!!")

# the below detection works since the only arm we support is M1,
# otherwise much more complex detections are needed
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" && ${MACOS_ARCHITECTURE} MATCHES "arm")
        #message(WARNING "Current ARCH is macos arm!!!")
        set(CM256_IS_ARM TRUE)
else()
        set(CM256_IS_ARM FALSE)
endif()

add_library(whistFEC_cm256 STATIC
        cm256.cpp
        gf256.cpp
        )

if(NOT ${CM256_IS_ARM})
        add_subdirectory(avx2)
        add_subdirectory(ssse3)
        target_link_libraries(whistFEC_cm256 whistFEC_cm256_avx2)
        target_link_libraries(whistFEC_cm256 whistFEC_cm256_ssse3)
endif()
