#pragma once

#include <optional>
#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include "api/units/data_rate.h"
#include "rtc_base/checks.h"
#include <string>
#include <set>
#include <deque>
using namespace std;

// Data with timestamp and value
struct Data {
    double time;
    double value;
};

// A data structure to get statistics of data in a sliding window
struct SlidingWindowStat {
    string name;  // name for the object, only for debug

    double window_size;        // sliding window size, unit: sec
    double sample_period;      // how often samples get inserted, unit: sec
    double last_sampled_time;  // last time a smaple got inserted, init: sec
    long long sum;  // a running sum of all samples inside the window, convert to int to avoid float
                    // error accumulation
    const int float_scale = 1000 * 1000;  // scale float to int with this factor

    multiset<double> ordered_data;  // stores values in order
    deque<pair<Data, multiset<double>::iterator>>
        q;  // a queue of original data, with iterator pointing to the ordered_data

    // init or re-init the data structure
    void init(string name_in, double window_size_in, double sample_period_in) {
        name = name_in;
        window_size = window_size_in;
        sample_period = sample_period_in;
        last_sampled_time = 0;
        clear();
    }

    // try to insert a sample into the end of sliding window
    void insert(double current_time, double value) {
        // when insert, need to respect the sample_period
        if (current_time - last_sampled_time < sample_period) {
            return;
        }
        last_sampled_time = current_time;

        if (0 && name == "packet_loss") {
            printf("[FEC_CONTROLLER]sampled value=%.2f for %s\n", value * 100.0, name.c_str());
        }

        //  maintain the data structure
        Data data;
        data.time = current_time;
        data.value = value;
        sum += value * float_scale;
        auto it = ordered_data.insert(value);
        q.push_back(make_pair(data, it));
    }

    // clear the data structure
    void clear() {
        sum = 0;
        q.clear();
        ordered_data.clear();
    }

    // get num of samples stored
    int size() { return (int)q.size(); }

    // peek the data at front of the sliding window
    Data peek_front() { return q.front().first; }

    // pop the data at front of the sliding window
    void pop_front() {
        // maintain the data structure when doing pop
        ordered_data.erase(q.front().second);
        sum -= q.front().first.value * float_scale;
        q.pop_front();
    }

    // get the average of values inside sliding window
    double get_avg() { return sum * 1.0 / float_scale / size(); }

    // get the max value inside sliding window
    double get_max() { return *ordered_data.rbegin(); }

    // get the min value inside sliding window
    double get_min() { return *ordered_data.begin(); }

    // get the i percentage max value inside the sliding window
    // the fumction is inefficient for small i

    // TODO: there is a data structure that can calculate any i in log(n) time,
    // if the below code is not efficient enough, we can implement it.
    double get_i_percentage_max(int i) {
        assert(i >= 1 && i <= 100);
        int backward_steps = size() * (100 - i) / 100.0;
        backward_steps = min(backward_steps, size() - 1);
        auto it = ordered_data.rbegin();
        while (backward_steps--) {
            it++;
        }
        return *it;
    }

    // slide the front of window, drop all values which are too old
    void slide_window(double current_time) {
        while (size() > 0 && peek_front().time < current_time - window_size) {
            pop_front();
        }
    }
};

struct BandwithSaturateController
{
    public:
    SlidingWindowStat bwd_stat;
    const double time_wait_before_turn_off_saturate=30;
    const double diff_allowed_before_turn_off_saturate=0.3;

    bool saturate= false;
    webrtc::Timestamp last_turn_on_time = webrtc::Timestamp::MinusInfinity(); 

    BandwithSaturateController(){

    }

    void init(webrtc::Timestamp current_time)
    {
        last_turn_on_time = current_time;
        saturate= true;
        bwd_stat.init("bwd", time_wait_before_turn_off_saturate, 0.001);
    }

    void insert_bandwidth_sample(webrtc::Timestamp current_time, double bandwidth){
        bwd_stat.insert(current_time.ms()/1000.0, bandwidth);
        bwd_stat.slide_window(current_time.ms()/1000.0);
        if(current_time - last_turn_on_time > webrtc::TimeDelta::Seconds(time_wait_before_turn_off_saturate)) {

            double low= bwd_stat.get_min();
            double high= bwd_stat.get_max();

            //printf("<<<%f %f %f>>>\n", (high-low)/high, high,low);
            if( (high - low)/high <= diff_allowed_before_turn_off_saturate){
                saturate = false;
            }
        }
    }

    void turn_on(webrtc::Timestamp current_time){
        RTC_CHECK(saturate == false);
        saturate =true;
        last_turn_on_time = current_time;
        bwd_stat.clear();
    }

    bool get_saturate_state(){
        return saturate;
    }
};


struct CCSharedState {

    shared_ptr<BandwithSaturateController> bwd_saturate_controller;
    const double k_smaller_clamp_min= 1.f;
    const double k_clamp_min= 6.f;
    const double k_startup_duration=6;
    const double k_increase_ratio=0.12;

    static constexpr double loss_hold_threshold=0.0999;
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

    double shoot_ratio_raw_100=0;
    double shoot_ratio_100=0;
    double shoot_rate=0;

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


