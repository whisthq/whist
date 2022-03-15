if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    #message(WARNING "Current ARCH is LINUx!!!")
    add_compile_options("-mavx2")
    add_compile_options("-Wno-restrict")
    add_compile_options("-O2")
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
    add_compile_options("-O2")
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

add_library(whistFEC_cm256 STATIC
        cm256.cpp
        gf256.cpp
        )

#set_property(TARGET whistTools PROPERTY
#        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
