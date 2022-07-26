/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file fec_controller.cpp
 * @brief contains implementation of the fec controller
 */

#include <stack>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <string>

#include <whist/core/whist.h>

extern "C" {
#include <whist/utils/clock.h>
#include <whist/network/network_algorithm.h>
#include <whist/fec/fec.h>
#include <whist/debug/debug_console.h>
#include <whist/debug/protocol_analyzer.h>
#include "fec_controller.h"
}

/*
============================
Defines
============================
*/
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

        if (LOG_FEC_CONTROLLER && name == "packet_loss") {
            LOG_INFO("[FEC_CONTROLLER]sampled value=%.2f for %s\n", value * 100.0, name.c_str());
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

    // get the i percentage max value inside the sliding window
    // the fumction is inefficient for small i

    // TODO: there is a data structure that can calculate any i in log(n) time,
    // if the below code is not efficient enough, we can implement it.
    double get_i_percentage_max(int i) {
        FATAL_ASSERT(i >= 1 && i <= 100);
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

// the controller class to get base fec ratio based on measured packet loss, the base fec ratio
// tries to protect both:
// 1. inherent packet loss of network
// 2. the portion of packet loss that WCC fails/refuses to react on
struct BaseRatioController {
    // sliding window for statistics of packet loss
    SlidingWindowStat packet_loss_stat;

    // a queue to hold packet loss data, before the data is considered safe to push into the
    // sliding window
    deque<Data> packet_loss_record_queue;

    double init_time;                      // time of init, unit s
    double last_increase_bandwith_time;    // the last time we get a bandwith increase operation, s
    double last_fec_ratio;                 // previously calculated fec ratio
    double last_non_hit_min_bitrate_time;  // the last time we get a bitrate that is not min, unit s

    // init or re-init the controller
    void init(double current_time) {
        const double packet_loss_stat_window =
            45.0;  // size of sliding window of packet loss, unit s
        const double packet_loss_sample_period =
            1.5;  // min period between packet loss is sampled, unit s

        packet_loss_stat.init("packet_loss", packet_loss_stat_window, packet_loss_sample_period);
        packet_loss_record_queue.clear();

        init_time = current_time;
        last_fec_ratio = INITIAL_FEC_RATIO;

        last_non_hit_min_bitrate_time = 0.0;
        last_increase_bandwith_time = 0.0;
    }

    // feed info into the controller, the controller tries best to measure the packet loss of only:
    // inherent packet loss of network + the portion of packet loss that WCC fails/refuses to react
    // on
    void feed_network_info(double current_time, double packet_loss, int current_bitrate,
                           int min_bitrate, WccOp op) {
        // during startup the protocol is not stable, we don't record any samples for the first 5s
        const double START_COOLDOWN = 5.0;  // NOLINT
        // small wait time for various purposes...
        const double SMALL_WAIT_TIME = 3.0;  // NOLINT

        // slide the window to drop old data
        packet_loss_stat.slide_window(current_time);

        // update last time we are NOT in min_bitrate
        if (current_bitrate != min_bitrate) {
            last_non_hit_min_bitrate_time = current_time;
        }

        // skip recording info for the first 5s
        if (current_time - init_time < START_COOLDOWN) {
            return;
        }

        // if we have hit the min bitrate for quite a while, we have to live with the current
        // packet loss, since the bitrate can't decrease anymore
        if (current_time - last_non_hit_min_bitrate_time > SMALL_WAIT_TIME) {
            packet_loss_stat.insert(current_time, packet_loss);
            return;
        }

        // a bandwidth increase operation is a strong indication that the network is not congested
        // at the moment, so we trust the packet loss measured
        if (op == WCC_INCREASE_BWD) {
            packet_loss_stat.insert(current_time, packet_loss);
            packet_loss_record_queue.clear();
            last_increase_bandwith_time = current_time;
        }

        // we only rely on the logic below if there is no bandwith increase operation recently
        if (current_time - last_increase_bandwith_time < SMALL_WAIT_TIME) return;

        // record the packet_loss info into a temp queue,
        // we only move the data into the paclet_loss_stat structure, if there is no bandwidth
        // decrease option in the follow 3s
        Data data;
        data.time = current_time;
        data.value = packet_loss;
        packet_loss_record_queue.push_back(data);

        // if we got a bandwitdh decrease operation, that means there is likely a congestion, all
        // the data in the temp queue is not reliable
        if (op == WCC_DECREASE_BWD) {
            packet_loss_record_queue.clear();
        }

        // if a data inside the temp queue has exist for 3s without seeing a bandwitdh decrease
        // operation, we belive the packet loss measure is reliable and move it into the
        // packet_loss_stat
        while (packet_loss_record_queue.size() > 0 &&
               current_time - packet_loss_record_queue.front().time >= SMALL_WAIT_TIME) {
            packet_loss_stat.insert(current_time, packet_loss_record_queue.front().value);
            packet_loss_record_queue.pop_front();
        }
    }

    // get base_fec_ratio based on the data inside packet_loss_stat
    double get_base_fec_ratio(double current_time) {
        // base fec ratio only reacts to packet loss at this max value
        const double PACKET_LOSS_MAX = 0.1;  // NOLINT
        // use how many unit of fec ratio to protection one packet loss unit
        const double BASE_FEC_PROTECTING_FACTOR = 1.2;  // NOLINT

        // if there is no data, don't update value, just return the old one
        if (packet_loss_stat.size() == 0) {
            return last_fec_ratio;
        }

        // get max and avg packet loss inside the sliding window
        double max_packet_loss = packet_loss_stat.get_max();
        double avg_packet_loss = packet_loss_stat.get_avg();

        double packet_loss = max_packet_loss;
        if (LOG_FEC_CONTROLLER) {
            static double last_print_time = 0;
            if (current_time - last_print_time > 1.0) {
                last_print_time = current_time;
                LOG_INFO("[FEC_CONTROLLER] avg=%.3f max=%.3f size=%d sum=%lld\n", avg_packet_loss,
                         max_packet_loss, packet_loss_stat.size(), packet_loss_stat.sum);
            }
        }

        // try to clip ill value by average
        if (packet_loss > 4 * avg_packet_loss + 0.05) {
            packet_loss = 4 * avg_packet_loss + 0.05;
        }

        // try to clip ill value by 90th percentage
        if (packet_loss_stat.size() > 10) {
            double the_90per_value = packet_loss_stat.get_i_percentage_max(90);
            if (packet_loss > the_90per_value + 0.05) {
                packet_loss = the_90per_value + 0.05;
            }
        }

        // clip out-of-range value
        packet_loss = min(packet_loss, PACKET_LOSS_MAX);

        // cal the new_fec_ratio based on packet_loss
        double new_fec_ratio = packet_loss * BASE_FEC_PROTECTING_FACTOR;

        // save value to last_fec_ratio
        last_fec_ratio = new_fec_ratio;

        return new_fec_ratio;
    }
};

// the controller class to get extra fec ratio, which tries to reduce the risk of packet loss cause
// by the bandwith probing of congestion control
struct ExtraRatioController {
    double last_used_extra_fec_time;  // the last time extra fec is used，unit s
    bool extra_fec_in_use;            // if extra fec is currently used,

    // the 2 vars below are only valid when extra_fec_in_use is true
    double extra_fec_ratio;  // current extra fec ratio
    int extend_cnt_left;     // how many times left to allow a extend period of extra_fec_ratio

    // init or re-init the controller
    void init() {
        last_used_extra_fec_time = 0;
        extra_fec_in_use = false;
    }

    // tries best to keep track of the bandwith probing of congestion control and network
    // congestion, calculate extra_fec_ratio to reducing the congestion caused by bandwith probing
    void fec_control_feed_bandwith_prob_info(double current_time, int old_bitrate,
                                             int current_bitrate, WccOp op) {
        // uppder bound of extra fec, which is same as the ratio of max bandwith increase
        const double UPPER_BOUND = 1 + MAX_INCREASE_PERCENTAGE / 100.0;  // NOLINT

        // how long does the extra fec protection last, unit s
        const double EXTRA_FEC_LAST_TIME = 3.0;  // NOLINT
        // if we get a congestion during extra fec protection, the protection time above can be
        // extended for 1 time
        const double EXTRA_FEC_MAX_EXTEND_CNT = 1;  // NOLINT

        // if the congestion control wants probe new bandwith by increase bitrate,
        // we don't incrase video bitrate directly, instead we pad those increase portion with FEC
        if (op == WCC_INCREASE_BWD && current_bitrate > old_bitrate) {
            // extra_fec_ratio is same as the bandwith increase ratio
            extra_fec_ratio = current_bitrate * 1.0 / old_bitrate - 1.0;

            // clip the value for robustness
            extra_fec_ratio = min(extra_fec_ratio, UPPER_BOUND);

            // indicate extra fec is in use
            extra_fec_in_use = true;
            // how many times left to allow extending of the extra_fec_period
            extend_cnt_left = EXTRA_FEC_MAX_EXTEND_CNT;
            // record current time
            last_used_extra_fec_time = current_time;
            if (LOG_FEC_CONTROLLER) {
                LOG_INFO("[FEC_CONTROLLER] enable extra fec!!! %.3f\n", extra_fec_ratio);
            }
        } else if (extra_fec_in_use) {
            // if we get a WCC_DECREASE_BWD during the protection of extra fec,
            // we extend the period of extra_fec to wait for congestion disapear
            // by refresh the timer
            if (op == WCC_DECREASE_BWD && extend_cnt_left > 0) {
                extend_cnt_left--;
                last_used_extra_fec_time = current_time;
                if (LOG_FEC_CONTROLLER) {
                    LOG_INFO("[FEC_CONTROLLER] refreshed extra fec!!! %.3f\n", extra_fec_ratio);
                }
            }

            // if we havn't see WCC_DECREASE_BWD/congestion for EXTRA_FEC_LAST_TIME,
            // then there is eigher no congestion or the WCC fails/refuses to react,
            // so we turn of the extra fec, and let base fec react on this later
            if (current_time - last_used_extra_fec_time > EXTRA_FEC_LAST_TIME) {
                extra_fec_in_use = 0;
                if (LOG_FEC_CONTROLLER) {
                    LOG_INFO("[FEC_CONTROLLER] disable extra fec!!!\n");
                }
            }
        }
    }

    // return the calculated extra_fec_ratio
    double fec_controller_get_extra_fec_ratio(double current_time) {
        if (extra_fec_in_use) {
            return extra_fec_ratio;
        } else {
            return 0.0;
        }
    }
};

// struct of fec controller
struct FECController {
    // controller for base fec ratio
    BaseRatioController *base_ratio_controller;
    // controller for extra fec ratio
    ExtraRatioController *extra_ratio_controller;
    // another sliding window to calculate latency
    SlidingWindowStat *latency_stat;
};

/*
============================
Public Function Implementations
============================
*/

void *create_fec_controller(double current_time) {
    FECController *fec_controller = (FECController *)malloc(sizeof(FECController));
    mlock((void*) fec_controller, sizeof(FECController));

    // size of sliding window for latency
    const double latency_stat_window = 30.0;
    // sample period for latency
    const double latency_sample_period = 0.3;

    // allocation the objects if not done previously
    fec_controller->base_ratio_controller = new BaseRatioController;
    fec_controller->extra_ratio_controller = new ExtraRatioController;
    fec_controller->latency_stat = new SlidingWindowStat;
    mlock((void*)fec_controller->base_ratio_controller, sizeof(BaseRatioController));
    mlock((void*)fec_controller->extra_ratio_controller, sizeof(ExtraRatioController));
    mlock((void*)fec_controller->latency_stat, sizeof(SlidingWindowStat));

    // init the controllers and latency sliding window
    fec_controller->base_ratio_controller->init(current_time);
    fec_controller->extra_ratio_controller->init();
    fec_controller->latency_stat->init("latency", latency_stat_window, latency_sample_period);

    return fec_controller;
}

void destroy_fec_controller(void *controller) {
    FECController *fec_controller = (FECController *)controller;
    munlock((void*)fec_controller->base_ratio_controller, sizeof(BaseRatioController));
    munlock((void*)fec_controller->extra_ratio_controller, sizeof(ExtraRatioController));
    munlock((void*)fec_controller->latency_stat, sizeof(SlidingWindowStat));
    munlock((void*) fec_controller, sizeof(FECController));

    delete fec_controller->base_ratio_controller;
    delete fec_controller->extra_ratio_controller;
    delete fec_controller->latency_stat;
    free(fec_controller);
}

void fec_controller_feed_latency(void *controller, double current_time, double latency) {
    FECController *fec_controller = (FECController *)controller;
    fec_controller->latency_stat->slide_window(current_time);
    fec_controller->latency_stat->insert(current_time, latency * MS_IN_SECOND);
}

void fec_controller_feed_info(void *controller, double current_time, WccOp op, double packet_loss,
                              int old_bitrate, int current_bitrate, int min_bitrate,
                              bool saturate_bandwidth) {
    FECController *fec_controller = (FECController *)controller;
    fec_controller->base_ratio_controller->feed_network_info(current_time, packet_loss,
                                                             current_bitrate, min_bitrate, op);
    fec_controller->extra_ratio_controller->fec_control_feed_bandwith_prob_info(
        current_time, old_bitrate, current_bitrate, op);

    if (LOG_FEC_CONTROLLER) {
        if (op == WCC_INCREASE_BWD) {
            LOG_INFO(
                "[FEC_CONTROLLER]WCC Bitrate Increase! old_bitrate=%d new_bitrate=%d "
                "saturate_bandwidth=%d\n",
                old_bitrate, current_bitrate, saturate_bandwidth);
        } else if (op == WCC_DECREASE_BWD) {
            LOG_INFO(
                "[FEC_CONTROLLER]Bitrate Decrease! old_bitrate=%d new_bitrate=%d\n "
                "saturate_bandwidth=%d",
                old_bitrate, current_bitrate, saturate_bandwidth);
        }
    }
}

double fec_controller_get_total_fec_ratio(void *controller, double current_time, double old_value) {
    FECController *fec_controller = (FECController *)controller;

    const double inf = 9999.0;
    // min ratio for fec to take effect
    const double FEC_MIN_START = 0.005;  // NOLINT
    // min step for an immediate update of fec value
    const double FEC_MIN_STEP = 0.005;  // NOLINT

    // get base_fec_ratio and extra_fec_ratio
    double base_fec_ratio = fec_controller->base_ratio_controller->get_base_fec_ratio(current_time);
    double extra_fec_ratio =
        fec_controller->extra_ratio_controller->fec_controller_get_extra_fec_ratio(current_time);

    // get adjusted base_fec_ratio that only acts on the portion that is not extra fec ratio
    double adjusted_base_fec_ratio = (1.0 - extra_fec_ratio) * base_fec_ratio;

    // then add adjusted base_fec_ratio with extra_fec_ratio
    double total_fec_ratio = adjusted_base_fec_ratio + extra_fec_ratio;

    // if fec ratio is really small just clip it to zero, to reduce the overhead by integer rounding
    if (total_fec_ratio < FEC_MIN_START) {
        total_fec_ratio = 0;
    }

    // robustness clip
    total_fec_ratio = min(total_fec_ratio, MAX_FEC_RATIO);

    // get the 90 percentage max latency
    double latency = inf;
    if (fec_controller->latency_stat->size() > 0) {
        latency = fec_controller->latency_stat->get_i_percentage_max(90);  // get 90 percentage max
    }

    // if lantency lower than this, (almost) disable fec
    const double LATENCY_LOWER_BOUND = 20.0;  // NOLINT
    // if latency higher than this, fully utilize fec
    const double LATENCY_UPPER_BOUND = 40.0;  // NOLINT

    double fec_latency_fix_factor = 1.0;  // the total_rec_ratio will be multiply by this value
    // if latency < lower_bound,
    if (latency < LATENCY_LOWER_BOUND) {
        fec_latency_fix_factor = 0;
    }
    // if lower_bound <latency < upper_bound, scale the factor linearly according to latecny
    else if (latency < LATENCY_UPPER_BOUND) {
        fec_latency_fix_factor =
            (latency - LATENCY_LOWER_BOUND) / (LATENCY_UPPER_BOUND - LATENCY_LOWER_BOUND);
    }

    // save orignal value for easier debug
    double total_fec_ratio_original = total_fec_ratio;
    // calculate new value based on latency fix factor
    total_fec_ratio = total_fec_ratio_original * fec_latency_fix_factor;

    // we punish the ratio to 0.01 at minimum, since retransmission of packet might lose as well,
    // it's helpfull to have some small spare packet
    if (total_fec_ratio_original > 0.01) {
        total_fec_ratio = max(0.01, total_fec_ratio);
    }

    // only update the value if exceed min_step
    if (fabs(total_fec_ratio - old_value) < FEC_MIN_STEP) {
        total_fec_ratio = old_value;
    } else {
        whist_analyzer_record_current_fec_info(PACKET_VIDEO, base_fec_ratio, extra_fec_ratio,
                                               total_fec_ratio_original, total_fec_ratio);
        if (!LOG_FEC_CONTROLLER) {
            LOG_INFO_RATE_LIMITED(5, 1,
                                  "[FEC_CONTROLLER] base_fec_ratio=%.3f "
                                  "extra_fec_ratio=%.3f total_fec_ratio_original=%.3f "
                                  "total_fec_ratio=%.3f old_value=%.3f\n",
                                  base_fec_ratio, extra_fec_ratio, total_fec_ratio_original,
                                  total_fec_ratio, old_value);
        } else {
            LOG_INFO(
                "[FEC_CONTROLLER] base_fec_ratio=%.3f "
                "extra_fec_ratio=%.3f total_fec_ratio_original=%.3f total_fec_ratio=%.3f "
                "old_value=%.3f\n",
                base_fec_ratio, extra_fec_ratio, total_fec_ratio_original, total_fec_ratio,
                old_value);
        }
    }

    // handle fec override by debug console
    if (get_debug_console_override_values()->video_fec_ratio >= 0.0) {
        total_fec_ratio = get_debug_console_override_values()->video_fec_ratio;
    }

    return total_fec_ratio;
}
