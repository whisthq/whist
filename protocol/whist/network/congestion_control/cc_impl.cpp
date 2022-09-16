extern "C"
{
#include "cc_interface.h"
}

#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include "common_fixes.h"
#include <cstddef>
#include <iostream>
#include <memory>
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

class CongestionCongrollerImpl:CongestionCongrollerInterface
{
    std::unique_ptr<webrtc::DelayBasedBwe> delay_based_bwe;
    std::unique_ptr<webrtc::SendSideBandwidthEstimation> send_side_bwd;


    public:
    CongestionCongrollerImpl()
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
        delay_based_bwe= std::make_unique<webrtc::DelayBasedBwe> (&ft,nullptr,nullptr);
        send_side_bwd= std::make_unique<webrtc::SendSideBandwidthEstimation> (&ft,nullptr);


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

        printf("zong!!\n");

    }

   ~CongestionCongrollerImpl() override
   {
   }

   void feed_packet(double send_time_ms, double arrival_time_ms, double current_incoming_bitrate) override{

   }

   double get_target_bitrate() override{
    fprintf(stderr,"it works!!!\n");
    return 0;
   }
};


void *create_congestion_controller()
{
  return new CongestionCongrollerImpl();
}

void destory_congestion_controller(void *p)
{
  CongestionCongrollerImpl * object= (CongestionCongrollerImpl*)p;
}
