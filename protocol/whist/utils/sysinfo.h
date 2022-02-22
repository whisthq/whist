#ifndef SYSINFO_H
#define SYSINFO_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file sysinfo.h
 * @brief This file contains all code that logs client hardware specifications.
============================
Usage
============================

Call the respective functions to log a device's OS, model, CPU, RAM, etc.
*/

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
void print_os_info(void);

/**
 * @brief                          Prints the OS model and build version
 */
void print_model_info(void);

/**
 * @brief                          Prints the monitors
 */
void print_monitors(void);

/**
 * @brief                          Prints the total RAM and used RAM
 */
void print_ram_info(void);

/**
 * @brief                          Prints the CPU specs, vendor and usage
 */
void print_cpu_info(void);

/**
 * @brief                          Prints the total storage and the available
 * storage
 */
void print_hard_drive_info(void);

/**
 * @brief                          Prints the memory
 */
void print_memory_info(void);

/**
 * @brief                          Get the instantaneous CPU usage in percentage points.
 *                                 The function only supports Linux. On other platforms, it
 *                                 returns -1.
 * @returns                        Returns -1 on failure, the CPU usage on success
 */
double get_cpu_usage(void);

#endif  // SYS_INFO
