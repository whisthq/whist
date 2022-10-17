
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file plotter.h
 * @brief APIs for plotting graphs
============================
Usage
============================
The plotter API is backended by two implementations:
1. the basic plotter, where you can call start_sampling(), stop_sampling(), and export_to_file()
   afterwrad.
2. the stream plotter, once created, it will peridically flush samples onto disk.

Both of them accept samples by the insert_sample() API.

Call these functions from anywhere within client where they're needed.
*/

#pragma once

#include "debug_flags.h"

/**
 * @brief                          Init the plotter sub system
 *
 * @param filename                 If provided, the plotter will be initalized as a stream plotter
 *                                 which flushes data peridocally onto this file on disk.
 *                                 Otherwise, the plotter will be initalized as a basic plotter.
 *
 * @note                           this function doesn't mean to be thread safe, it should be called
 *                                 before other plotter APIs, and called only once
 */
void whist_plotter_init(const char* filename);

/**
 * @brief                          Start or restart sampling of samples
 *
 * @note                           All previous existing samples are cleared
 *                                 by calling this function.
 *                                 Only for non-stream plotter
 */
void whist_plotter_start_sampling(void);

/**
 * @brief                          Stops sampling
 *
 * @note                           e.g stop to avoid waste of CPU and memory used on sampling
 *                                 Only for non-stream plotter
 */
void whist_plotter_stop_sampling(void);

/**
 * @brief                          Insert a sample to the plotter
 *
 * @param label                    the label of point. Points with same label are considered as in a
 *                                 same dataset
 * @param x                        self-explained x value, the plotter doesn't try to interpret the
 *                                 meaning, it's only treated a double value
 * @param y                        self-explained y value
 * @note                           Has no effect if the plotter is not initalized.
 */
void whist_plotter_insert_sample(const char* label, double x, double y);

/**
 * @brief                          Export the samples to a json format file. This function is a
 *                                 wrapper of whist_plotter_export().
 *
 * @param filename                 name of the file to export
 * @note                           Only for non-stream plotter
 * @returns                        0 on success
 */
int whist_plotter_export_to_file(const char* filename);

/**
 * @brief                          Destory plotter, release resouces
 *
 * @note                           safe to call regardless of whether init() has been called.
 *                                 this function doesn't mean to be thread safe, and should
 *                                 be called at the very end, i.e. before the protocol quits
 */
void whist_plotter_destroy(void);
