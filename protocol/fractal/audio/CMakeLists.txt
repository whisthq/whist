add_library(fractalAudio STATIC
        audiodecode.c
        audiocapture.h
        audiodecode.h
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(fractalAudio PRIVATE alsacapture.c alsacapture.h)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(fractalAudio PRIVATE wasapicapture.c wasapicapture.h)
endif ()

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(fractalAudio PRIVATE audioencode.h audioencode.c)
endif()

set_property(TARGET fractalAudio PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(STATUS ${LIB})
    target_link_libraries(fractalAudio ${LIB})
endforeach(LIB)