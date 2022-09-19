#pragma once

void * create_congestion_controller(void);
void destory_congestion_controller(void*);

#ifdef __cplusplus
extern "C++" {

#include <optional>
#include <vector>

struct PacketInfo
{
    double send_time;
    double recv_time;
};
struct CCInput
{
   double current_timestamp_ms;
   std::vector<PacketInfo> packets;

   std::optional<double> start_bitrate;
   std::optional<double> min_bitrate;
   std::optional<double> max_bitrate;
   std::optional<double> packet_loss;
   //std::optional<double> ack_bitrate;

   std::optional<double> incoming_bitrate;

   std::optional<double> rtt_ms;
};

struct CCOutput
{
    std::optional<double> target_bitrate;
};


class CongestionCongrollerInterface
{
    public:
    CongestionCongrollerInterface(){};
    virtual ~CongestionCongrollerInterface() {};
    virtual CCOutput feed_info(CCInput input)=0;
    virtual CCOutput process_interval(CCInput input)=0;
    //virtual double get_target_bitrate() =0;
};
}
#endif


