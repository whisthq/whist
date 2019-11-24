/*
 * This file contains the implementation of the headers used as part of the
 * streaming protocol.

 Protocol version: 1.0
 Last modification: 11/24/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "fractal.h" // header file for this file

// Winsock Library if Windows is defined
#if defined(_WIN32)
	#include <winsock2.h> // lib for socket programming on windows
	#pragma comment(lib, "ws2_32.lib")
	#pragma warning(disable: 4201)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*** CLIENT FUNCTIONS START ***/
/*** CLIENT FUNCTIONS END ***/

/*** SERVER FUNCTIONS START ***/
/// @brief Initialize the server
/// @param[in] cfg Fractal instance configuration. May be `NULL` to use FRACTAL_DEFAULTS.
/// @returns error if an init error appears, else FRACTAL_OK
//static FractalStatus FractalInit(FractalConfig *cfg) {}










/// @brief Shuts down the server instance.
/// @param[in] none
/// @returns none
//static void FractalDestroy() {}














/*** SERVER FUNCTIONS END ***/

#ifdef __cplusplus
}
#endif

// renable Windows warning
#if defined(_WIN32)
	#pragma warning(default: 4201)
#endif
