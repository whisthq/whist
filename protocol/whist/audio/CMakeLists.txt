add_library(whistAudio STATIC
        audiodecode.c
        audiocapture.h
        audiodecode.h
        )

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_sources(whistAudio PRIVATE alsacapture.c alsacapture.h)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(whistAudio PRIVATE wasapicapture.c wasapicapture.h)
endif()

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(whistAudio PRIVATE audioencode.h audioencode.c)
endif()

set_property(TARGET whistAudio PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

foreach(LIB ${FFMPEG_LIBS_PATHS})
    message(VERBOSE "linking lib for whistAudio: ${LIB}")
    target_link_libraries(whistAudio ${LIB})
endforeach()
