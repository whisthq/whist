#pragma once

#include <optional>
#include "api/units/timestamp.h"
#include "api/units/data_rate.h"
#include "rtc_base/checks.h"

struct CCSharedState {
    const double k_smaller_clamp_min= 1.f;
    const double k_clamp_min= 6.f;
    const double k_startup_duration=6;
    const double k_increase_ratio=0.12;

    static constexpr double loss_hold_threshold=0.08;
    static constexpr double loss_decrease_threshold=0.10;

    webrtc::Timestamp current_time = webrtc::Timestamp::MinusInfinity();

    webrtc::DataRate current_bitrate = webrtc::DataRate::MinusInfinity();
    webrtc::DataRate max_bitrate = webrtc::DataRate::MinusInfinity();
    webrtc::DataRate min_bitrate = webrtc::DataRate::MinusInfinity();

    std::optional<webrtc::DataRate> ack_bitrate;
    webrtc::Timestamp first_process_time=webrtc::Timestamp::MinusInfinity();

    // count how many est (valid samples) we have in the estimator
    int est_cnt_=0;
    webrtc::Timestamp last_est_time = webrtc::Timestamp::MinusInfinity();

    webrtc::DataRate last_sample = webrtc::DataRate::MinusInfinity();
    webrtc::Timestamp last_sample_time = webrtc::Timestamp::MinusInfinity();

    double loss_ratio=0;

    bool in_slow_increase(){
        return est_cnt_>=2;
    }

    double get_increase_ratio(){
        if(est_cnt_==0) return k_increase_ratio;
        else if(est_cnt_==1) return 0.03;
        //else if(est_cnt_==2) return 0.02;
        return 0.015;
    }

    double get_kdown();
    double get_kup(){
        const double k_up_default=0.0087;
        return k_up_default;
    }
};

extern CCSharedState cc_shared_state;
