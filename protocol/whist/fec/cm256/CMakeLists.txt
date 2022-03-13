if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_compile_options("-mavx2")
    add_compile_options("-Wno-restrict")

    message(WARNING "Current ARCH is LINUx!!!")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(${MACOS_ARCHITECTURE} MATCHES "arm")
        #add_compile_options("-mavx2")
        message(WARNING "Current ARCH is macos arm!!!")
    elseif( ${MACOS_ARCHITECTURE} MATCHES "x86" )
        add_compile_options("-mavx2")
        message(WARNING "Current ARCH is macos x86 or x64!!!")
    else()
        message(FATAL "unknown Darwin architecture: ${MACOS_ARCHITECTURE} ")
    endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if(MSVC)
        add_compile_options("/arch:AVX2")
        message(WARNING "Current ARCH is windows with MSVC!!!")
    else()
        add_compile_options("-mavx2")
        message(WARNING "Current ARCH is windows with non-MSVC!!!")

    endif()
endif()

add_library(whistFEC_cm256 STATIC
        cm256.cpp
        gf256.cpp
        )

#set_property(TARGET whistTools PROPERTY
#        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
