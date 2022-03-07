#pragma once
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file audio_path.h
 * @brief This file contains an implement of data path from network to audio device
============================
Usage
============================

call audio_path_init() before calling anything else.
call push_to_audio_path() to push a frame into the audio path
call pop_from_audio_path() to pop a frame from the audio path

Note: frame pushed into audio path might be reordered, delay, droped or duplicated by the controling
logic inside, to hide network jitter or avoid audio device queue empty

*/

/*
============================
Public Functions
============================
*/

/**
 * @brief                          init the audio path subsystem
 *
 * @param decoded_bytes_per_frame  the expected size of a decoded frame
 * @returns                        zero on success, otherwise fail
 */
int audio_path_init(int decoded_bytes_per_frame);

/**
 * @brief                          put a frame/packet into the audio path
 *
 * @param id                       id of the frame
 * @param buf                      data of the frame
 * @param size                     size of the frame
 * @returns                        zero on success, otherwise fail
 */
int push_to_audio_path(int id, unsigned char *buf, int size);

/**
 * @brief                          pop a frame/packet from the audio path
 *
 * @param buf                      data of the frame
 * @param size                     size of the frame
 * @param device_queue_bytes       the length of device queue in bytes
 * @returns                        zero on success, otherwise fail
 */
int pop_from_audio_path(unsigned char *buf, int *size, int device_queue_bytes);
