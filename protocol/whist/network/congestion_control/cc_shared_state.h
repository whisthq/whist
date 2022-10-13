#pragma once

#include "api/units/timestamp.h"
struct CCSharedState
{
    bool g_in_slow_increase= false;
    const double kWhistClampMin= 5.f;
    const double g_startup_duration=6;
    const double g_increase_ratio=0.12;
    double max_bitrate=-1;
    double current_bitrate_ratio=1;

    double ack_bitrate= -1;
    webrtc::Timestamp first_process_time=webrtc::Timestamp::MinusInfinity();

};

extern CCSharedState cc_shared_state;
