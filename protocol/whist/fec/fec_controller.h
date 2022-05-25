/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file fec_controller.h
 * @brief contains functions to feed info into fec controller, and get fec ratio from fec controller
 */

#pragma once

#define ENABLE_FEC false
#define INITIAL_FEC_RATIO 0.05

// return value structure for fec_controller_get_total_fec_ratio()
typedef struct {
    // base fec depending on packet loss measuring
    double base_fec_ratio;
    // extra fec for protect bandwitdh probing
    double extra_fec_ratio;
    // the orignal total_fec_ratio calculated by base_fec_ratio and extra_fec_ratio without adjust
    double total_fec_ratio_original;

    // the final total_fec_ratio, this is the value actually used for fec. All values above are
    // exposed to outside just for easier debuging
    double total_fec_ratio;
} FECInfo;

/**
 * @brief                          init or re-init the fec controller
 *
 * @param current_time             comparable current_time in sec
 */
void fec_controller_init(double current_time);

/**
 * @brief                          feed latency info into the fec controller
 *
 * @param current_time             comparable current_time in sec
 * @param latency                  latency in sec
 *
 * @note                           this is a dedicated function for latency feed, since latency
 *                                 is always avaliable while others are not
 */
void fec_controller_feed_latency(double current_time, double latency);

/**
 * @brief                          feed all other info into fec controller
 *
 * @param current_time             comparable current_time in sec
 * @param op                       current operation in WCC
 * @param packet_loss              current packet loss
 * @param old_bitrate              old bitrate suggested by WCC
 * @param current_bitrate          current bitrate suggested by WCC
 * @param min_bitrate              the min bitrate for current resolution and dpi
 */
void fec_controller_feed_info(double current_time, WccOp op, double packet_loss, int old_bitrate,
                              int current_bitrate, int min_bitrate);

/**
 * @brief                          get the fec ratio computed by fec_controller
 *
 * @param current_time             comparable current_time in sec
 * @param old_value                old value of fec
 *
 * @returns                        total fec ratio, along with a few other values for debug
 *
 * @note                           when old value is not -1, and new calculated value is close
 *                                 enough to the old value, the returned value might just be the
 *                                 old value. This is for avoiding uncessary parameter change
 *                                 sending over the internet.
 */
FECInfo fec_controller_get_total_fec_ratio(double current_time, double old_value);
