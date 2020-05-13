add_library(fractalVideo STATIC
        screencapture.h
        videodecode.h
        videodecode.h
        videodecode.c
        videoencode.h
        )

if (${UNIX} AND NOT ${APPLE})
    target_sources(fractalVideo PRIVATE x11capture.c x11capture.h)
elseif(${WIN32})
    target_sources(fractalVideo PRIVATE dxgicapture.c dxgicapture.h)
endif ()