/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_REMOTE_BITRATE_ESTIMATOR_AIMD_RATE_CONTROL_H_
#define MODULES_REMOTE_BITRATE_ESTIMATOR_AIMD_RATE_CONTROL_H_

#include <stdint.h>

#include "common_fixes.h"
//#include "absl/types/optional.h"
#include "api/field_trials_view.h"
#include "api/transport/network_types.h"
#include "api/units/data_rate.h"
#include "api/units/timestamp.h"
#include "modules/congestion_controller/goog_cc/link_capacity_estimator.h"
#include "modules/remote_bitrate_estimator/include/bwe_defines.h"
#include "rtc_base/experiments/field_trial_parser.h"

namespace webrtc {
// A rate control implementation based on additive increases of
// bitrate when no over-use is detected and multiplicative decreases when
// over-uses are detected. When we think the available bandwidth has changes or
// is unknown, we will switch to a "slow-start mode" where we increase
// multiplicatively.
class AimdRateControl {
 public:
  explicit AimdRateControl(const FieldTrialsView* key_value_config);
  AimdRateControl(const FieldTrialsView* key_value_config, bool send_side);
  ~AimdRateControl();

  // Returns true if the target bitrate has been initialized. This happens
  // either if it has been explicitly set via SetStartBitrate/SetEstimate, or if
  // we have measured a throughput.
  bool ValidEstimate() const;
  void SetStartBitrate(DataRate start_bitrate);
  void SetMinBitrate(DataRate min_bitrate);
  TimeDelta GetFeedbackInterval() const;

  // Returns true if the bitrate estimate hasn't been changed for more than
  // an RTT, or if the estimated_throughput is less than half of the current
  // estimate. Should be used to decide if we should reduce the rate further
  // when over-using.
  bool TimeToReduceFurther(Timestamp at_time,
                           DataRate estimated_throughput) const;
  // As above. To be used if overusing before we have measured a throughput.
  bool InitialTimeToReduceFurther(Timestamp at_time) const;

  DataRate LatestEstimate() const;
  void SetRtt(TimeDelta rtt);
  DataRate Update(const RateControlInput* input, Timestamp at_time);
  void SetInApplicationLimitedRegion(bool in_alr);
  void SetEstimate(DataRate bitrate, Timestamp at_time);
  void SetNetworkStateEstimate(
      const absl::optional<NetworkStateEstimate>& estimate);

  // Returns the increase rate when used bandwidth is near the link capacity.
  double GetNearMaxIncreaseRateBpsPerSecond() const;
  // Returns the expected time between overuse signals (assuming steady state).
  TimeDelta GetExpectedBandwidthPeriod() const;

 private:
  enum class RateControlState { kRcHold, kRcIncrease, kRcDecrease };

  friend class GoogCcStatePrinter;
  // Update the target bitrate based on, among other things, the current rate
  // control state, the current target bitrate and the estimated throughput.
  // When in the "increase" state the bitrate will be increased either
  // additively or multiplicatively depending on the rate control region. When
  // in the "decrease" state the bitrate will be decreased to slightly below the
  // current throughput. When in the "hold" state the bitrate will be kept
  // constant to allow built up queues to drain.
  void ChangeBitrate(const RateControlInput& input, Timestamp at_time);

  DataRate ClampBitrate(DataRate new_bitrate) const;
  DataRate MultiplicativeRateIncrease(Timestamp at_time,
                                      Timestamp last_ms,
                                      DataRate current_bitrate) const;
  DataRate AdditiveRateIncrease(Timestamp at_time, Timestamp last_time) const;
  void UpdateChangePeriod(Timestamp at_time);
  void ChangeState(const RateControlInput& input, Timestamp at_time);

  DataRate min_configured_bitrate_;
  DataRate max_configured_bitrate_;
  DataRate current_bitrate_;
  DataRate latest_estimated_throughput_;
  LinkCapacityEstimator link_capacity_;
  absl::optional<NetworkStateEstimate> network_estimate_;
  RateControlState rate_control_state_;
  Timestamp time_last_bitrate_change_;
  Timestamp time_last_bitrate_decrease_;
  Timestamp time_first_throughput_estimate_;
  bool bitrate_is_initialized_;
  double beta_;
  bool in_alr_;
  TimeDelta rtt_;
  const bool send_side_;
  const bool in_experiment_;
  // Allow the delay based estimate to only increase as long as application
  // limited region (alr) is not detected.
  const bool no_bitrate_increase_in_alr_;
  // Use estimated link capacity lower bound if it is higher than the
  // acknowledged rate when backing off due to overuse.
  const bool estimate_bounded_backoff_;
  // If false, uses estimated link capacity upper bound *
  // `estimate_bounded_increase_ratio_` as upper limit for the estimate.
  FieldTrialFlag disable_estimate_bounded_increase_{"Disabled"};
  FieldTrialParameter<double> estimate_bounded_increase_ratio_{"ratio", 1.0};
  FieldTrialParameter<bool> ignore_throughput_limit_if_network_estimate_{
      "ignore_acked", false};
  FieldTrialParameter<bool> increase_to_network_estimate_{"immediate_incr",
                                                          false};
  FieldTrialParameter<bool> ignore_network_estimate_decrease_{"ignore_decr",
                                                              false};
  absl::optional<DataRate> last_decrease_;
  FieldTrialOptional<TimeDelta> initial_backoff_interval_;
  FieldTrialFlag link_capacity_fix_;

  Timestamp first_process_time= Timestamp::MinusInfinity();
};
}  // namespace webrtc

#endif  // MODULES_REMOTE_BITRATE_ESTIMATOR_AIMD_RATE_CONTROL_H_
