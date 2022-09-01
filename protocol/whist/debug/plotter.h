
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file plotter.h
 * @brief APIs for plotting graphs
============================
Usage
============================

Call these functions from anywhere within client where they're
needed.
*/

#pragma once

#include "debug_flags.h"

/**
 * @brief                          Init the plotter sub system
 *
 */
void whist_plotter_init(void);

/**
 * @brief                          Start or restart sampling of samples
 *
 * @note                           All previous existing samples are cleared
 *                                 by calling this function
 */
void whist_plotter_start_sampling(void);

/**
 * @brief                          Stops sampling
 *
 * @note                           e.g stop to avoid waste of CPU and memory used on sampling
 */
void whist_plotter_stop_sampling(void);

/**
 * @brief                          Insert a sample to the plotter
 *
 * @param label                    the label of point. Points with same label are considered as in a
 * same dataset
 * @param x                        self-explained x value, the plotter doesn't try to interpret the
 * meaning, it's only treated a double value
 * @param y                        self-explained y value
 * @note                           Has no effect if the plotter is not in a started status
 */
void whist_plotter_insert_sample(const char* label, double x, double y);

// only expose this function to c++
#ifdef __cplusplus
extern "C++" {
#include <string>

/**
 * @brief                          Export the samples to a json format string
 *
 * @returns                        json format string, that can be imported by the plotter.py
 */
std::string whist_plotter_export();
}
#endif

/**
 * @brief                          Export the samples to a json format file. This function is a
 *                                 wrapper of whist_plotter_export().
 *
 * @param filename                 name of the file to export
 *
 */
void whist_plotter_export_to_file(const char* filename);

/**
 * @brief                          Export the samples to a json format string. This function is a C
 *                                 wrapper to whist_plotter_export().
 *
 * @param out_s                    The point to the buffer to use to save the JSON string
 * @param max_size                 The size of the buffer
 *
 */
void whist_plotter_export_c(char* out_s, size_t max_size);
