/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file fec_controller.h
 * @brief contains functions to feed info into fec controller, and get fec ratio from fec controller
 */

#pragma once

#define ENABLE_FEC false
#define INITIAL_FEC_RATIO 0.05

/**
 * @brief                          Create FEC Controller
 *
 * @param current_time             Comparable current_time in sec
 *
 * @returns                        The created FEC controller
 */
void* create_fec_controller(double current_time);

/**
 * @brief                          Destroy FEC Controller
 *
 * @param fec_controller           The FEC controller to destory
 */
void destroy_fec_controller(void* fec_controller);

/**
 * @brief                          feed latency info into the fec controller
 *
 * @param fec_controller           The FEC controller
 * @param current_time             comparable current_time in sec
 * @param latency                  latency in sec
 *
 * @note                           this is a dedicated function for latency feed, since latency
 *                                 is always avaliable while others are not
 */
void fec_controller_feed_latency(void* fec_controller, double current_time, double latency);

/**
 * @brief                          feed all other info into fec controller
 *
 * @param fec_controller           The FEC controller
 * @param current_time             comparable current_time in sec
 * @param op                       current operation in WCC
 * @param packet_loss              current packet loss
 * @param old_bitrate              old bitrate suggested by WCC
 * @param current_bitrate          current bitrate suggested by WCC
 * @param min_bitrate              the min bitrate for current resolution and dpi
 * @param saturate_bandwidth       saturate_bandwidth flag from WCC, only for debug
 */
void fec_controller_feed_info(void* fec_controller, double current_time, WccOp op,
                              double packet_loss, int old_bitrate, int current_bitrate,
                              int min_bitrate, bool saturate_bandwidth);

/**
 * @brief                          get the fec ratio computed by fec_controller
 *
 * @param fec_controller           The FEC controller
 * @param current_time             comparable current_time in sec
 * @param old_value                old value of fec
 *
 * @returns                        total fec ratio
 *
 * @note                           when old value is not -1, and new calculated value is close
 *                                 enough to the old value, the returned value might just be the
 *                                 old value. This is for avoiding uncessary parameter change
 *                                 sending over the internet.
 */
double fec_controller_get_total_fec_ratio(void* fec_controller, double current_time,
                                          double old_value);
