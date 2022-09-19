#include <optional>
#include "api/units/data_rate.h"
#include "whist/logging/logging.h"
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
        webrtc::FieldTrials ft("");
        /*
        webrtc::FieldTrials ft("WebRTC-Bwe-EstimateBoundedIncrease/"
        "ratio:0.85,ignore_acked:true,immediate_incr:false/"
        "WebRTC-DontIncreaseDelayBasedBweInAlr/Enabled/"
        "WebRTC-BweBackOffFactor/Enabled-0.9/"
        "WebRTC-Bwe-TrendlineEstimatorSettings/sort:false/"
        "WebRTC-BweAimdRateControlConfig/initial_backoff_interval:200/"
        "WebRTC-Bwe-MaxRttLimit/limit:2s,floor:100bps/"
        );*/
        delay_based_bwe= std::make_unique<webrtc::DelayBasedBwe> (&ft,nullptr,nullptr);
        send_side_bwd= std::make_unique<webrtc::SendSideBandwidthEstimation> (&ft,nullptr);

        /*
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
        */
    }

   ~CongestionCongrollerImpl() override
   {
   }

  virtual CCOutput feed_info(CCInput input) override
  {
      webrtc::Timestamp current_time= webrtc::Timestamp::Seconds( input.current_timestamp_ms);
      
      //RTC_CHECK(input.packets.size()==1);

      if(input.rtt_ms.has_value())
      {
        RTC_CHECK(input.rtt_ms.has_value() && input.rtt_ms.value() >0);
        send_side_bwd->UpdateRtt(webrtc::TimeDelta::Millis(input.rtt_ms.value()), current_time);
      }

      std::optional<webrtc::DataRate> start_rate;
      if(input.start_bitrate.has_value())
      {
         start_rate=webrtc::DataRate::BitsPerSec(input.start_bitrate.value());
      }

      RTC_CHECK(input.min_bitrate.has_value() && input.max_bitrate.has_value());

      send_side_bwd->SetBitrates(start_rate, webrtc::DataRate::BitsPerSec(input.min_bitrate.value()),
                                     webrtc::DataRate::BitsPerSec(input.max_bitrate.value()),  current_time);

      RTC_CHECK(input.packet_loss.has_value());
      send_side_bwd->UpdatePacketsLostDirect(input.packet_loss.value(), current_time);

      if(input.incoming_bitrate.has_value())
      {
        send_side_bwd->SetAcknowledgedRate(webrtc::DataRate::BitsPerSec(input.incoming_bitrate.value()),current_time);
      }

      CCOutput output;
      webrtc::DataRate loss_based_target_rate = send_side_bwd->target_rate();
      output.target_bitrate=loss_based_target_rate.bps();
      return output;
  }
  virtual CCOutput process_interval(CCInput input) override
  {
   // CCOutput output;
    webrtc::Timestamp current_time= webrtc::Timestamp::Seconds( input.current_timestamp_ms);

    send_side_bwd->UpdateEstimate(current_time);
    
    CCOutput output;
    webrtc::DataRate loss_based_target_rate = send_side_bwd->target_rate();
    output.target_bitrate=loss_based_target_rate.bps();

    return output;
  }
  /*
  double get_target_bitrate() override{
    fprintf(stderr,"it works!!!\n");
    return 0;
   }*/
};


void *create_congestion_controller()
{
  return new CongestionCongrollerImpl();
}

void destory_congestion_controller(void *p)
{
  CongestionCongrollerImpl * object= (CongestionCongrollerImpl*)p;
}
