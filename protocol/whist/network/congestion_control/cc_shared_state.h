#pragma once

#include <optional>
#include "api/units/timestamp.h"
#include "api/units/data_rate.h"
struct CCSharedState
{
    const double k_clamp_min= 5.f;
    const double k_startup_duration=6;
    const double k_increase_ratio=0.12;

    bool in_slow_increase= false;

    webrtc::DataRate max_bitrate= webrtc::DataRate::MinusInfinity();
    double current_bitrate_ratio=1;

    std::optional<webrtc::DataRate> ack_bitrate;
    webrtc::Timestamp first_process_time=webrtc::Timestamp::MinusInfinity();

};

extern CCSharedState cc_shared_state;
