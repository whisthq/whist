#ifndef SDL_androidsem_h_
#define SDL_androidsem_h_

#include "SDL_egl.h"
#include "SDL_mutex.h"
#include "SDL_syswm.h"

#include "begin_code.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

extern SDL_sem *Android_PauseSem, *Android_ResumeSem;
extern SDL_mutex *Android_ActivityMutex;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#include "close_code.h"

#endif