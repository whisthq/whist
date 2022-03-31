# this lib is like 10 times slower when compiled with -O0
# it's better to compile as -O2 even for debug version

if(NOT MSVC)
        #message(WARNING "Current compiler is NOT MSVC!!!")
        # the below line triggers MSVC complaint: "/RTC1 incompatible with /O2"
        # TODO fix this
        add_compile_options("$<$<CONFIG:DEBUG>:-O2>")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    #message(WARNING "Current ARCH is LINUx!!!")
    add_compile_options("-mssse3")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(${MACOS_ARCHITECTURE} MATCHES "arm")
        #message(WARNING "Current ARCH is macos arm!!!")
    elseif( ${MACOS_ARCHITECTURE} MATCHES "x86" )
        #message(WARNING "Current ARCH is macos x86 or x64!!!")
        add_compile_options("-mavx2")
    else()
        message(FATAL "unknown Darwin architecture: ${MACOS_ARCHITECTURE} ")
    endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if(MSVC)
        #message(WARNING "Current ARCH is windows with MSVC!!!")
        add_compile_options("/arch:AVX2")
    else()
        #message(WARNING "Current ARCH is windows with non-MSVC!!!")
        add_compile_options("-mavx2")
    endif()
endif()

add_subdirectory(avx2)

add_library(whistFEC_cm256 STATIC
        cm256.cpp
        gf256.cpp
        )

target_link_libraries(whistFEC_cm256 whistFEC_cm256_avx2)
