#include <deque>
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
#include "cc_shared_state.h"
#include <cstddef>
#include <iostream>
#include <memory>
#include <queue>
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
    webrtc::Timestamp first_ts= webrtc::Timestamp::MinusInfinity();

    long long last_group_id=-1;
    PacketInfo last_group_packet;

    //const int rtt_window_size=2;
    //SlidingWindowStat rtt_stat;

    deque<PacketInfo> packet_history;

    int last_seq=0;

    public:
    CongestionCongrollerImpl()
    {
        //rtt_stat.init("window_rtt", rtt_window_size, 0.005);
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

        cc_shared_state.bwd_saturate_controller = make_shared<BandwithSaturateController>();
    }

   ~CongestionCongrollerImpl() override
   {
      delete ft;
   }

  virtual CCOutput feed_info(CCInput input) override
  {
      webrtc::Timestamp current_time= webrtc::Timestamp::Millis( input.current_time_ms);
      cc_shared_state.current_time=current_time;
      /*
      ========================================
      maintain packet history and shoot_ratio
      ========================================
      */

      RTC_CHECK(input.packets.size()==1);
      if(input.packets[0].seq > last_seq)
      {
        auto & packet = input.packets[0];

        double window_size_ms=200;

        packet_history.push_back(input.packets[0]);
        while(packet_history.back().depature_time_ms - packet_history.front().depature_time_ms > window_size_ms){
          packet_history.pop_front();
        }
        double shoot_sum= (packet_history.back().bytes_so_far - packet_history[0].bytes_so_far);
        double target_sum= 1;
        for(int i=1;i<(int)packet_history.size();i++){
             target_sum+= packet_history[i].remote_target_bps/8.0 * (packet_history[i].depature_time_ms - packet_history[i-1].depature_time_ms)*0.001;
        }

        cc_shared_state.shoot_ratio_100 = shoot_sum/target_sum *100;

        if(cc_shared_state.shoot_ratio_100>150) cc_shared_state.shoot_ratio_100=150;

        //fprintf(stderr,"<%f %f %d %d %f>\n", shoot_sum, target_sum, (int)packet_history.size(), (int)(packet_history.back().depature_time_ms - packet_history.front().depature_time_ms), cc_shared_state.shoot_ratio_100);

        whist_plotter_insert_sample("shoot_ratio", get_timestamp_sec(), cc_shared_state.shoot_ratio_100);
      }


      /*
      ========================================
      do first time handling, mainly related to start bitrate
      ========================================
      */
      if(first_time)
      {
         RTC_CHECK(input.min_bitrate.has_value() && input.max_bitrate.has_value() && input.start_bitrate.has_value());
         cc_shared_state.first_process_time= current_time;
         cc_shared_state.bwd_saturate_controller->init(current_time);
         std::optional<webrtc::DataRate> start_rate;
         //printf("<<%f>>\n", input.start_bitrate.value());
         start_rate=webrtc::DataRate::BitsPerSec(input.start_bitrate.value());
         delay_based_bwe->SetMinBitrate(webrtc::DataRate::BitsPerSec(input.min_bitrate.value())); //need this before set startbitrate
         delay_based_bwe->SetStartBitrate(start_rate.value());

         send_side_bwd->SetBitrates(start_rate, webrtc::DataRate::BitsPerSec(input.min_bitrate.value()),
                              webrtc::DataRate::BitsPerSec(input.max_bitrate.value()),  current_time);

         first_time=false;
         first_ts=current_time;
      }

      /*
      ========================================
      feed rtt into both loss-based and delay-based controller
      ========================================
      */
      {
        double rtt_ms=50;
        if(input.srtt_ms.has_value()){
            RTC_CHECK( input.srtt_ms.value() >0);
            rtt_ms=input.srtt_ms.value();
        }
        send_side_bwd->UpdateRtt(webrtc::TimeDelta::Millis(rtt_ms), current_time);
        delay_based_bwe->OnRttUpdate(webrtc::TimeDelta::Millis(rtt_ms));
      }

      /*
      ========================================
      feed min-max bitrate into both loss-based and delay-based controller
      ========================================
      */
      {
        RTC_CHECK(input.min_bitrate.has_value() && input.max_bitrate.has_value() && input.start_bitrate.has_value());
        send_side_bwd->SetBitrates(std::nullopt, webrtc::DataRate::BitsPerSec(input.min_bitrate.value()),
                                      webrtc::DataRate::BitsPerSec(input.max_bitrate.value()),  current_time);
        /*send_side_bwd->SetBitrates(std::nullopt, webrtc::DataRate::BitsPerSec(input.min_bitrate.value()),
                                      webrtc::DataRate::BitsPerSec(input.min_bitrate.value()+100),  current_time);*/

        delay_based_bwe->SetMinBitrate(webrtc::DataRate::BitsPerSec(input.min_bitrate.value()));
        cc_shared_state.min_bitrate = webrtc::DataRate::BitsPerSec(input.min_bitrate.value());
        cc_shared_state.max_bitrate = webrtc::DataRate::BitsPerSec(input.max_bitrate.value());
      }

      /*
      ========================================
      feed packet loss ratio to loss based controller
      ========================================
      */
      {
        RTC_CHECK(input.packet_loss.has_value());
        RTC_CHECK(input.packet_loss.value()>=0 && input.packet_loss.value()<=1);

        double packet_loss= input.packet_loss.value();
        //packet_loss-=0.05; // might have problem, maybe better to change inside loss based controller
        packet_loss = max<double>(0,packet_loss);

        send_side_bwd->UpdatePacketsLost(1e6*packet_loss, 1e6,current_time);

        cc_shared_state.loss_ratio = packet_loss;
        //send_side_bwd->UpdatePacketsLostDirect(input.packet_loss.value(), current_time);
      }

      /*
      ========================================
      feed incoming bitrate infointo bitrate_esitmator
      ========================================
      */
      {
        webrtc::TransportPacketsFeedback report;
        RTC_CHECK(input.packets.size()==1);
        report.feedback_time=current_time;
        report.packet_feedbacks.emplace_back();
        report.packet_feedbacks[0].receive_time= webrtc::Timestamp::Millis(input.packets[0].arrival_time_ms);
        report.packet_feedbacks[0].sent_packet.send_time= webrtc::Timestamp::Millis(input.packets[0].depature_time_ms);
        report.packet_feedbacks[0].sent_packet.size = webrtc::DataSize::Bytes(input.packets[0].packet_size);
        acknowledged_bitrate_estimator_->IncomingPacketFeedbackVector(report.SortedByReceiveTime());
      }

      /*
      ========================================
      feed bitrate estimation into loss-based controller
      ========================================
      */
      auto acknowledged_bitrate = acknowledged_bitrate_estimator_->bitrate();
      if(acknowledged_bitrate.has_value())
      {
        send_side_bwd->SetAcknowledgedRate(webrtc::DataRate::BitsPerSec(acknowledged_bitrate->bps()),current_time);
        {
            static double last_plot_time_ms=0;
            if(input.current_time_ms - last_plot_time_ms > 5)
            {
                whist_plotter_insert_sample("ack_bitrate", get_timestamp_sec(), acknowledged_bitrate->bps()/1000.0/100.0);
                last_plot_time_ms = input.current_time_ms;
            }
        }

        //incoming_optional = webrtc::DataRate::BitsPerSec(input.incoming_bitrate.value());
        cc_shared_state.ack_bitrate = acknowledged_bitrate;
      }


      /*
      ========================================
      group packet and feed grouped packet's arrival and departure into delay-based controller
      ========================================
      */
      webrtc::DelayBasedBwe::Result result;
      {
        webrtc::TransportPacketsFeedback grouped_report;
                optional<webrtc::DataRate> dummy_probe;
          optional<webrtc::NetworkStateEstimate> dummy_est;

        RTC_CHECK(input.packets.size()==1);
        long long current_group_id= input.packets[0].group_id;

        if(current_group_id > last_group_id)
        {
          if(last_group_id!=-1)
          {
            grouped_report.feedback_time=current_time;
            grouped_report.packet_feedbacks.emplace_back();
            grouped_report.packet_feedbacks[0].receive_time= webrtc::Timestamp::Millis(last_group_packet.arrival_time_ms);
            grouped_report.packet_feedbacks[0].sent_packet.send_time= webrtc::Timestamp::Millis(last_group_packet.depature_time_ms);
            grouped_report.packet_feedbacks[0].sent_packet.size = webrtc::DataSize::Bytes(last_group_packet.packet_size);

            result=delay_based_bwe->IncomingPacketFeedbackVector(
                grouped_report, acknowledged_bitrate, dummy_probe, dummy_est,
                false);
          }

          last_group_packet= input.packets[0];
          last_group_id = current_group_id;
        }
      }

      /*
      ========================================
      feed delay-based controller's target bitrate back into loss-based controller
      ========================================
      */
      if (result.updated) {
            send_side_bwd->UpdateDelayBasedEstimate(current_time,
                                                            result.target_bitrate);
            whist_plotter_insert_sample("delay_based_bwd", get_timestamp_sec(), result.target_bitrate.bps()/1000.0/100.0);
      }
      //whist_plotter_insert_sample("delay_last_est", get_timestamp_sec(), delay_based_bwe->last_estimate().bps()/1000.0/100.0);

      /*
      ========================================
      return final target bitrate to outside. (not really used by outside, at the moment)
      ========================================
      */
      //send_side_bwd->UpdateEstimate(current_time);  // seems not necessary, the original webrtc code doesn't call it in feedback
      CCOutput output;
      webrtc::DataRate loss_based_target_rate = send_side_bwd->target_rate();
      output.target_bitrate=loss_based_target_rate.bps();
      return output;
  }
  virtual CCOutput process_interval(double current_time_ms) override
  {
      webrtc::Timestamp current_time= webrtc::Timestamp::Millis(current_time_ms);
      /*
      ========================================
      do period update of loss-base controller, and return final bitrate to outside
      ========================================
      */
      send_side_bwd->UpdateEstimate(current_time);
      CCOutput output;
      webrtc::DataRate loss_based_target_rate = send_side_bwd->target_rate();
      output.target_bitrate=loss_based_target_rate.bps();

      cc_shared_state.current_bitrate = loss_based_target_rate;
      //whist_plotter_insert_sample("current_bitrate_ratio", get_timestamp_sec(), cc_shared_state.current_bitrate_ratio*100);


      cc_shared_state.bwd_saturate_controller->insert_bandwidth_sample(  current_time, loss_based_target_rate.bps());

      output.bandwitdth_saturation=  cc_shared_state.bwd_saturate_controller->get_saturate_state();


      /*
      if(current_time - cc_shared_state.first_process_time > webrtc::TimeDelta::Seconds(10)){
        int sec= (current_time - cc_shared_state.first_process_time).seconds();
        if(sec%10==0||sec%10==1)
        {
          output.bandwitdth_saturation = true;
          output.bandwitdth_saturation = false;
        }
        else output.bandwitdth_saturation = false;
      } else{
        output.bandwitdth_saturation = true;
      }*/

      whist_plotter_insert_sample("saturate", get_timestamp_sec(), output.bandwitdth_saturation *-5);

      //output.bandwitdth_saturation = true;
      return output;
  }
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