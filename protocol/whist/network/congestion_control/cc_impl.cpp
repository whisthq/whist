#include <optional>
#include "api/transport/network_types.h"
#include "api/units/data_rate.h"
#include "api/units/data_size.h"
#include "whist/debug/plotter.h"
#include "whist/logging/logging.h"
#include "whist/utils/clock.h"

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

#include "modules/congestion_controller/goog_cc/acknowledged_bitrate_estimator_interface.h"

class CongestionCongrollerImpl:CongestionCongrollerInterface
{
    std::unique_ptr<webrtc::DelayBasedBwe> delay_based_bwe;
    std::unique_ptr<webrtc::SendSideBandwidthEstimation> send_side_bwd;
    std::unique_ptr<webrtc::AcknowledgedBitrateEstimatorInterface> acknowledged_bitrate_estimator_;
    webrtc::FieldTrials * ft;
    bool first_time=true;
    public:
    CongestionCongrollerImpl()
    {
        webrtc::field_trial::InitFieldTrialsFromString("");
        ft= new webrtc::FieldTrials("");
        /*
        webrtc::FieldTrials ft("WebRTC-Bwe-EstimateBoundedIncrease/"
        "ratio:0.85,ignore_acked:true,immediate_incr:false/"
        "WebRTC-DontIncreaseDelayBasedBweInAlr/Enabled/"
        "WebRTC-BweBackOffFactor/Enabled-0.9/"
        "WebRTC-Bwe-TrendlineEstimatorSettings/sort:false/"
        "WebRTC-BweAimdRateControlConfig/initial_backoff_interval:200/"
        "WebRTC-Bwe-MaxRttLimit/limit:2s,floor:100bps/"
        );*/
        delay_based_bwe= std::make_unique<webrtc::DelayBasedBwe> (ft,nullptr,nullptr);
        send_side_bwd= std::make_unique<webrtc::SendSideBandwidthEstimation> (ft,nullptr);
        delay_based_bwe->SetMinBitrate(webrtc::congestion_controller::GetMinBitrate());
        acknowledged_bitrate_estimator_=webrtc::AcknowledgedBitrateEstimatorInterface::Create(ft);
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
      delete ft;
   }

  virtual CCOutput feed_info(CCInput input) override
  {
      webrtc::Timestamp current_time= webrtc::Timestamp::Millis( input.current_time_ms);
      
      //RTC_CHECK(input.packets.size()==1);

      if(input.rtt_ms.has_value())
      {
        RTC_CHECK(input.rtt_ms.has_value() && input.rtt_ms.value() >0);
        send_side_bwd->UpdateRtt(webrtc::TimeDelta::Millis(input.rtt_ms.value()), current_time);
        delay_based_bwe->OnRttUpdate(webrtc::TimeDelta::Millis(input.rtt_ms.value()));
      }


      RTC_CHECK(input.min_bitrate.has_value() && input.max_bitrate.has_value() && input.start_bitrate.has_value());

      std::optional<webrtc::DataRate> start_rate;
      if(first_time)
      {
         start_rate=webrtc::DataRate::BitsPerSec(input.start_bitrate.value());
         delay_based_bwe->SetMinBitrate(webrtc::DataRate::BitsPerSec(input.min_bitrate.value()));
         delay_based_bwe->SetStartBitrate(start_rate.value());
         first_time=false;
      }

      send_side_bwd->SetBitrates(start_rate, webrtc::DataRate::BitsPerSec(input.min_bitrate.value()),
                                     webrtc::DataRate::BitsPerSec(input.max_bitrate.value()),  current_time);

      delay_based_bwe->SetMinBitrate(webrtc::DataRate::BitsPerSec(input.min_bitrate.value()));

      RTC_CHECK(input.packet_loss.has_value());
      RTC_CHECK(input.packet_loss.value()>=0 && input.packet_loss.value()<=1);
      send_side_bwd->UpdatePacketsLost(1e6*input.packet_loss.value(), 1e6,current_time);
      //send_side_bwd->UpdatePacketsLostDirect(input.packet_loss.value(), current_time);


      webrtc::TransportPacketsFeedback report;
      optional<webrtc::DataRate> dummy_probe;
      optional<webrtc::NetworkStateEstimate> dummy_est;

      RTC_CHECK(input.packets.size()==1);
      report.feedback_time=current_time;
      report.packet_feedbacks.emplace_back();
      report.packet_feedbacks[0].receive_time= webrtc::Timestamp::Millis(input.packets[0].arrival_time_ms);
      report.packet_feedbacks[0].sent_packet.send_time= webrtc::Timestamp::Millis(input.packets[0].depature_time_ms);
      report.packet_feedbacks[0].sent_packet.size = webrtc::DataSize::Bytes(input.packets[0].packet_size);


      acknowledged_bitrate_estimator_->IncomingPacketFeedbackVector(report.SortedByReceiveTime());

       auto acknowledged_bitrate = acknowledged_bitrate_estimator_->bitrate();
       /*
      optional<webrtc::DataRate> incoming_optional;
      if(input.incoming_bitrate.has_value())
      {
        send_side_bwd->SetAcknowledgedRate(webrtc::DataRate::BitsPerSec(input.incoming_bitrate.value()),current_time);
        incoming_optional = webrtc::DataRate::BitsPerSec(input.incoming_bitrate.value());
      }*/

      if(acknowledged_bitrate.has_value())
      {
        send_side_bwd->SetAcknowledgedRate(webrtc::DataRate::BitsPerSec(acknowledged_bitrate->bps()),current_time);
        whist_plotter_insert_sample("ack_bitrate", get_timestamp_sec(), acknowledged_bitrate->bps()/1000.0/100.0);
        //incoming_optional = webrtc::DataRate::BitsPerSec(input.incoming_bitrate.value());
      }

      webrtc::DelayBasedBwe::Result result= delay_based_bwe->IncomingPacketFeedbackVector(
      report, acknowledged_bitrate, dummy_probe, dummy_est,
      false);

      if (result.updated) {
            send_side_bwd->UpdateDelayBasedEstimate(report.feedback_time,
                                                            result.target_bitrate);
            whist_plotter_insert_sample("delay_based_bwd", get_timestamp_sec(), result.target_bitrate.bps()/1000.0/100.0);
      }
      //whist_plotter_insert_sample("fuck", get_timestamp_sec(), 100);

      //whist_plotter_insert_sample("delay_last_est", get_timestamp_sec(), delay_based_bwe->last_estimate().bps()/1000.0/100.0);

      CCOutput output;
      webrtc::DataRate loss_based_target_rate = send_side_bwd->target_rate();
      output.target_bitrate=loss_based_target_rate.bps();
      return output;
  }
  virtual CCOutput process_interval(double current_time_ms) override
  {
   // CCOutput output;
    webrtc::Timestamp current_time= webrtc::Timestamp::Millis(current_time_ms);

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
  delete object;
}
