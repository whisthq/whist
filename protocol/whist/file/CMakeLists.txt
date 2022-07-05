add_library(whistFile STATIC
        file_drop.c
        file_synchronizer.c
        )

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(whistFile PRIVATE file_download_macos.m)
else()
    target_sources(whistFile PRIVATE file_download.c)
endif()

set_property(TARGET whistFile PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
