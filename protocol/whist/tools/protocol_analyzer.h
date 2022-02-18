/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file protocol_analyzer.h
 * @brief TODO
============================
Usage
============================

Call these functions from anywhere within client where they're
needed.
*/

#ifndef PROTOCOL_ANALYZER_H
#define PROTOCOL_ANALYZER_H

/*
============================
Includes
============================
*/

#include <whist/network/network.h>
#include <whist/network/ringbuffer.h>

/*
============================
Defines
============================
*/

/*
============================
Public Functions
============================
*/

// init the analyzer
void whist_analyzer_init(void);

// record info related to the arrival of a segment
void whist_analyzer_record_segment(WhistSegment *packet);

// record a frame being ready
void whist_analyzer_record_ready_to_render(int type, int id, char *packet);

// record a nack is sent
void whist_analyzer_record_nack(int type, int id, int index);

// record a video frame is feed into decoder
void whist_analyzer_record_decode_video(void);

// record an audio frame is feed into decoder
void whist_analyzer_record_decode_audio(void);

// record a frame becomes the current rendering inside ringbuffer
void whist_analyzer_record_current_rendering(int type, int id, int overwrite_id);

// record a frame triggers audio queue full
void whist_analyzer_record_audio_queue_full(void);

// record a frame become pending rendering, i.e. waiting to be taken from decode thread
void whist_analyzer_record_pending_rendering(int type);

// record a ringbuffer reset
void whist_analyzer_record_reset_ringbuffer(int type, int from_id, int to_id);

// record a frame skip
void whist_analyzer_record_skip(int type, int from_id, int to_id);

// record fec is used for recover the frame
void whist_analyzer_record_fec_used(int type, int id);

// record a stream_reset, with the id as greatest_faild_id
void whist_analyzer_record_stream_reset(int type, int id);

// only expose this function to c++
#ifdef __cplusplus
extern "C++" {
#include <string>
std::string whist_analyzer_get_report(int type, int num, int skip, bool more_format);
}
#endif

#endif  // PROTOCOL_ANALYZER_H
