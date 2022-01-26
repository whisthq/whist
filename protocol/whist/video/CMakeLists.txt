add_library(whistVideo STATIC
        codec/decode.c
        video.c
        )

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(whistVideo PRIVATE
        transfercapture.c
        codec/encode.c
        codec/ffmpeg_encode.c
    )
    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_sources(whistVideo PRIVATE
            capture/linuxcapture.c
            capture/x11capture.c
            capture/nvidiacapture.c
            codec/nvidia_encode.c
            nvidia-linux/NvFBCUtils.c
            cudacontext.c
        )
    else()
        target_sources(whistVideo PRIVATE
            capture/windowscapture.c
        )
    endif()
endif()

set_property(TARGET whistVideo PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(whistVideo ${CMAKE_DL_LIBS})
