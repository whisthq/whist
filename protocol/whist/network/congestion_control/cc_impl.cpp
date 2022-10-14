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
        //FATAL_ASSERT(i >= 1 && i <= 100);
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

    const int rtt_window_size=2;
    SlidingWindowStat rtt_stat;

    public:
    CongestionCongrollerImpl()
    {
        rtt_stat.init("window_rtt", rtt_window_size, 0.005);
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
      do first time handling, mainly related to start bitrate
      ========================================
      */
      if(first_time)
      {
         RTC_CHECK(input.min_bitrate.has_value() && input.max_bitrate.has_value() && input.start_bitrate.has_value());
         cc_shared_state.first_process_time= current_time;
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
        RTC_CHECK(input.rtt_ms.has_value() && input.rtt_ms.value() >0);
        double rtt_ms= input.rtt_ms.value();
        double adjusted_rtt_ms = rtt_ms;
        send_side_bwd->UpdateRtt(webrtc::TimeDelta::Millis(rtt_ms), current_time);

        if(current_time-first_ts > webrtc::TimeDelta::Seconds( cc_shared_state.k_startup_duration))
        {
           rtt_stat.insert( input.current_time_ms/1000, input.rtt_ms.value());
           if(current_time-first_ts >webrtc::TimeDelta::Seconds( cc_shared_state.k_startup_duration + rtt_window_size/2))
           {
              double window_rtt = rtt_stat.get_i_percentage_max( 90);
              whist_plotter_insert_sample("window_rtt", get_timestamp_sec(), window_rtt);
              //adjusted_rtt_ms=max<double>(adjusted_rtt_ms,window_rtt);
           }
        }
        delay_based_bwe->OnRttUpdate(webrtc::TimeDelta::Millis(adjusted_rtt_ms));
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
        delay_based_bwe->SetMinBitrate(webrtc::DataRate::BitsPerSec(input.min_bitrate.value()));
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
        whist_plotter_insert_sample("ack_bitrate", get_timestamp_sec(), acknowledged_bitrate->bps()/1000.0/100.0);
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
      cc_shared_state.current_bitrate_ratio= loss_based_target_rate.bps()*1.0/cc_shared_state.max_bitrate.bps();

      whist_plotter_insert_sample("current_bitrate_ratio", get_timestamp_sec(), cc_shared_state.current_bitrate_ratio*100);

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
