# this lib is like 10 times slower when compiled with -O0
# it's better to compile as -O2 even for debug version

if(NOT MSVC)
        #message(WARNING "Current compiler is NOT MSVC!!!")
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

set(CM256_IS_X86 TRUE)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(${MACOS_ARCHITECTURE} MATCHES "arm")
        #message(WARNING "Current ARCH is macos arm!!!")
        set(CM256_IS_ARM FALSE)
    endif()
endif()

if(${CM256_IS_X86})
add_subdirectory(avx2)
add_subdirectory(ssse3)
endif()

add_library(whistFEC_cm256 STATIC
        cm256.cpp
        gf256.cpp
        )

if(${CM256_IS_X86})
        target_link_libraries(whistFEC_cm256 whistFEC_cm256_avx2)
        target_link_libraries(whistFEC_cm256 whistFEC_cm256_ssse3)
endif()
