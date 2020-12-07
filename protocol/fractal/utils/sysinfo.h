#ifndef SYSINFO_H
#define SYSINFO_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file sysinfo.h
 * @brief This file contains all code that logs client hardware specifications.
============================
Usage
============================

Call the respective functions to log a device's OS, model, CPU, RAM, etc.
*/

/*
============================
Includes
============================
*/

#ifdef _WIN32
#pragma warning(disable : 4201)
#include <D3D11.h>
#include <D3d11_1.h>
#include <DXGITYPE.h>
#include <dxgi1_2.h>
#include <windows.h>
#pragma comment(lib, "dxguid.lib")
#include <processthreadsapi.h>
#include <psapi.h>
#else
#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#else
#include <sys/sysinfo.h>
#endif
#include <sys/statvfs.h>
#include <sys/utsname.h>
#endif

#include <stdio.h>

#include "logging.h"

/*
============================
Defines
============================
*/

#define BYTES_IN_GB (1024.0 * 1024.0 * 1024.0)

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Prints the OS name,
 */
void print_os_info();

/**
 * @brief                          Prints the OS model and build version
 */
void print_model_info();

/**
 * @brief                          Prints the monitors
 */
void print_monitors();

/**
 * @brief                          Prints the total RAM and used RAM
 */
void print_ram_info();

/**
 * @brief                          Prints the CPU specs, vendor and usage
 */
void print_cpu_info();

/**
 * @brief                          Prints the total storage and the available
 * storage
 */
void print_hard_drive_info();

/**
 * @brief                          Prints the memory
 */
void print_memory_info();

#endif  // SYS_INFO