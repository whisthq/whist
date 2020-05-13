add_library(fractalAudio STATIC
        audiodecode.c
        audioencode.c
        audiocapture.h
        audiodecode.h
        audioencode.h)

if (${UNIX})
    target_sources(fractalAudio PRIVATE alsacapture.c alsacapture.h)
elseif(${WIN32})
    target_sources(fractalAudio PRIVATE wasapicapture.c wasapicapture.h)
endif ()