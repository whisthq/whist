add_library(fractalVideo STATIC
        screencapture.h
        videodecode.c
        videodecode.h
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(fractalVideo PRIVATE x11capture.c x11capture.h)
    target_sources(fractalVideo PRIVATE x11nvidiacapture.c x11nvidiacapture.h)
    target_sources(fractalVideo PRIVATE nvidia_encode.c nvidia_encode.h)
    target_sources(fractalVideo PRIVATE nvidiatransfercapture.c nvidiatransfercapture.h)
    target_sources(fractalVideo PRIVATE nvidia-linux/NvFBCUtils.c nvidia-linux/NvFBCUtils.h nvidia-linux/NvFBC.h)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if (CMAKE_CUDA_COMPILER)
        target_sources(fractalVideo PRIVATE dxgicudatransfercapture.cu)
        set_property(TARGET fractalVideo PROPERTY CUDA_ARCHITECTURES OFF)
    endif ()
    target_sources(fractalVideo PRIVATE dxgicudatransfercapture.h)
    target_sources(fractalVideo PRIVATE dxgicapture.c dxgicapture.h)
endif ()

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(fractalVideo PRIVATE transfercapture.c transfercapture.h)
    target_sources(fractalVideo PRIVATE videoencode.c videoencode.h)
endif ()

set_property(TARGET fractalVideo PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(fractalVideo ${CMAKE_DL_LIBS})
