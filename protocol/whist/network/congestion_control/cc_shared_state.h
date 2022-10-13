#pragma once

#include "api/units/timestamp.h"
struct CCSharedState
{
    const double k_clamp_min= 5.f;
    const double k_startup_duration=6;
    const double k_increase_ratio=0.12;

    bool in_slow_increase= false;

    double max_bitrate=-1;
    double current_bitrate_ratio=1;

    double ack_bitrate= -1;
    webrtc::Timestamp first_process_time=webrtc::Timestamp::MinusInfinity();

};

extern CCSharedState cc_shared_state;
