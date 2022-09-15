#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include <cstddef>
#include <iostream>
using namespace std;
#include "modules/congestion_controller/goog_cc/trendline_estimator.h"
#include "modules/congestion_controller/goog_cc/inter_arrival_delta.h"
#include "modules/congestion_controller/goog_cc/link_capacity_estimator.h"

#include "modules/remote_bitrate_estimator/aimd_rate_control.h"
#include "modules/congestion_controller/goog_cc/delay_based_bwe.h"

#include "modules/congestion_controller/goog_cc/send_side_bandwidth_estimation.h"

#include "api/transport/field_trial_based_config.h"
#include "rtc_base/checks.h"

#include "system_wrappers/include/field_trial.h"
#include "api/field_trials.h"

int example_call()
{
        webrtc::field_trial::InitFieldTrialsFromString("");
        
        webrtc::FieldTrials ft("WebRTC-Bwe-EstimateBoundedIncrease/"
      "ratio:0.85,ignore_acked:true,immediate_incr:false/"
      "WebRTC-DontIncreaseDelayBasedBweInAlr/Enabled/"
      "WebRTC-BweBackOffFactor/Enabled-0.9/"
      "WebRTC-Bwe-TrendlineEstimatorSettings/sort:false/"
      "WebRTC-BweAimdRateControlConfig/initial_backoff_interval:200/"
      "WebRTC-Bwe-MaxRttLimit/limit:2s,floor:100bps/"
      );
        //ftbc.InitFieldTrialsFromString("");
        int a;

        //RTC_DCHECK(1>2)<<"!!!!!";

        webrtc::TrendlineEstimator tl(&ft,nullptr);

        tl.UpdateTrendline(1, 1, 1, 1, 1);

        webrtc::InterArrivalDelta iad(webrtc::TimeDelta::Seconds(2));

        auto tt=webrtc::Timestamp::Seconds(2);
        auto dd=webrtc::TimeDelta::Seconds(2);
        iad.ComputeDeltas(tt, tt, tt, 111, &dd, &dd, &a);

        webrtc::LinkCapacityEstimator lce;
        lce.UpperBound();

        webrtc::AimdRateControl arc(&ft);


        webrtc::DelayBasedBwe dbb(&ft,nullptr,nullptr);

        webrtc::SendSideBandwidthEstimation ssbe(&ft,nullptr);

        printf("zong!!\n");
        return 0;
}
