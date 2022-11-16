/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/congestion_controller/goog_cc/inter_arrival_delta.h"

#include <algorithm>

#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include "common_fixes.h"
//#include "rtc_base/logging.h"

namespace webrtc {
static constexpr TimeDelta kBurstDeltaThreshold = TimeDelta::Millis(5);
static constexpr TimeDelta kMaxBurstDuration = TimeDelta::Millis(100);
constexpr TimeDelta InterArrivalDelta::kArrivalTimeOffsetThreshold;

InterArrivalDelta::InterArrivalDelta(TimeDelta send_time_group_length)
    : send_time_group_length_(send_time_group_length),
      current_timestamp_group_(),
      prev_timestamp_group_(),
      num_consecutive_reordered_packets_(0) {}

bool InterArrivalDelta::ComputeDeltas(Timestamp send_time,
                                      Timestamp arrival_time,
                                      Timestamp system_time,
                                      size_t packet_size,
                                      TimeDelta* send_time_delta,
                                      TimeDelta* arrival_time_delta,
                                      int* packet_size_delta) {
  bool calculated_deltas = false;
  //fprintf(stderr,"<%f>",send_time.us()/1000.0);
  if (current_timestamp_group_.IsFirstPacket()) {
    // We don't have enough data to update the filter, so we store it until we
    // have two frames of data to process.
    current_timestamp_group_.send_time = send_time;
    current_timestamp_group_.first_send_time = send_time;
    current_timestamp_group_.first_arrival = arrival_time;
  } else if (current_timestamp_group_.first_send_time > send_time) {
    // Reordered packet.
    return false;
  } else if (NewTimestampGroup(arrival_time, send_time)) {
    // First packet of a later send burst, the previous packets sample is ready.
    if (prev_timestamp_group_.complete_time.IsFinite()) {
      *send_time_delta =
          current_timestamp_group_.send_time - prev_timestamp_group_.send_time;
      *arrival_time_delta = current_timestamp_group_.complete_time -
                            prev_timestamp_group_.complete_time;

      whist_plotter_insert_sample("new group d", current_timestamp_group_.send_time.us()/1e6, -11);
      //whist_plotter_insert_sample("new group d with delta", current_timestamp_group_.send_time.us()/1e6, -10 + (arrival_time_delta->us() -send_time_delta->us())/1e3);

      whist_plotter_insert_sample("new group a", current_timestamp_group_.complete_time.us()/1e6, -19);
      //whist_plotter_insert_sample("new group a with delta", current_timestamp_group_.complete_time.us()/1e6, -15 + (arrival_time_delta->us() -send_time_delta->us())/1e3);

      //fprintf(stderr,"[[%f,%f;%f,%f]]\n",current_timestamp_group_.send_time.us()/1000.0,prev_timestamp_group_.send_time.us()/1000.0,current_timestamp_group_.complete_time.us()/1000.0,prev_timestamp_group_.complete_time.us()/1000.0);
      TimeDelta system_time_delta = current_timestamp_group_.last_system_time -
                                    prev_timestamp_group_.last_system_time;

      if (*arrival_time_delta - system_time_delta >=
          kArrivalTimeOffsetThreshold) {
        RTC_LOG(LS_WARNING)
            << "The arrival time clock offset has changed (diff = "
            << arrival_time_delta->ms() - system_time_delta.ms()
            << " ms), resetting.";
        Reset();
        return false;
      }
      if (*arrival_time_delta < TimeDelta::Zero()) {
        // The group of packets has been reordered since receiving its local
        // arrival timestamp.
        ++num_consecutive_reordered_packets_;
        if (num_consecutive_reordered_packets_ >= kReorderedResetThreshold) {
          RTC_LOG(LS_WARNING)
              << "Packets between send burst arrived out of order, resetting."
              << " arrival_time_delta" << arrival_time_delta->ms()
              << " send time delta " << send_time_delta->ms();
          Reset();
        }
        return false;
      } else {
        num_consecutive_reordered_packets_ = 0;
      }
      *packet_size_delta = static_cast<int>(current_timestamp_group_.size) -
                           static_cast<int>(prev_timestamp_group_.size);
      calculated_deltas = true;
    }
    prev_timestamp_group_ = current_timestamp_group_;
    // The new timestamp is now the current frame.
    current_timestamp_group_.first_send_time = send_time;
    current_timestamp_group_.send_time = send_time;
    current_timestamp_group_.first_arrival = arrival_time;
    current_timestamp_group_.size = 0;
  } else {
    current_timestamp_group_.send_time =
        std::max(current_timestamp_group_.send_time, send_time);
  }
  // Accumulate the frame size.
  current_timestamp_group_.size += packet_size;
  current_timestamp_group_.complete_time = arrival_time;
  current_timestamp_group_.last_system_time = system_time;

  return calculated_deltas;
}

// Assumes that `timestamp` is not reordered compared to
// `current_timestamp_group_`.
bool InterArrivalDelta::NewTimestampGroup(Timestamp arrival_time,
                                          Timestamp send_time) const {
  if (current_timestamp_group_.IsFirstPacket()) {
    return false;
  } else if (BelongsToBurst(arrival_time, send_time)) {
    return false;
  } else {
    if(ENABLE_WHIST_CHANGE){
      // since in whist we have group id to indicate the group, we don't need to group
      //  based on send time.
      return true;
    }
    return send_time - current_timestamp_group_.first_send_time >
           send_time_group_length_;
  }
}

bool InterArrivalDelta::BelongsToBurst(Timestamp arrival_time,
                                       Timestamp send_time) const {
  RTC_DCHECK(current_timestamp_group_.complete_time.IsFinite());
  TimeDelta arrival_time_delta =
      arrival_time - current_timestamp_group_.complete_time;
  TimeDelta send_time_delta = send_time - current_timestamp_group_.send_time;
  if (send_time_delta.IsZero())
    return true;
  if (ENABLE_WHIST_CHANGE){
    //trying to be compatible with out pacer
    //since whist pacer doesn't strictly send packets in burst of 5ms
    //if the finish time of last group is too close to next group
    //then the gradient might has problem, we merge them to reduce the issue.
    if(send_time_delta < TimeDelta::Millis(1.5f))
      return true;
  }
  TimeDelta propagation_delta = arrival_time_delta - send_time_delta;
  if(ENABLE_WHIST_CHANGE)
  {
      if (propagation_delta < TimeDelta::Zero() &&
      arrival_time_delta <= TimeDelta::Millis(9) &&
      arrival_time - current_timestamp_group_.first_arrival < kMaxBurstDuration){
        return true;
        //since whist pacer doesn't strictly send packets in burst of 5ms,
        //we need a loser condition than the 5ms kBurstDeltaThreshold
      }
      return false;
  }
  if (propagation_delta < TimeDelta::Zero() &&
      arrival_time_delta <= kBurstDeltaThreshold &&
      arrival_time - current_timestamp_group_.first_arrival < kMaxBurstDuration)
    return true;
  return false;
}

void InterArrivalDelta::Reset() {
  num_consecutive_reordered_packets_ = 0;
  current_timestamp_group_ = SendTimeGroup();
  prev_timestamp_group_ = SendTimeGroup();
}
}  // namespace webrtc