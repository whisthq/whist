add_library(fractalUtils STATIC
        aes.h
        clock.h
        logging.h
#        mac_utils.h
        sdlscreeninfo.h
        windows_utils.h
        aes.c
        clock.c
        logging.c
#        mac_utils.c
        sdlscreeninfo.c
#        windows_utils.c
        )

if (${UNIX} AND NOT ${APPLE})
    target_sources(fractalInput PRIVATE linuxinput.c linuxinput.h)
elseif(${WIN32})
    target_sources(fractalInput PRIVATE windowsinput.c windowsinput.h)
    #elseif(${APPLE})
    #    target_sources(fractalInput PRIVATE mac_clipboard.c clipboard_osx.m)
endif ()