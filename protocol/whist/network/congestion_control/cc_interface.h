#pragma once

void * create_congestion_controller(void);
void destory_congestion_controller(void*);

#ifdef __cplusplus
extern "C++" {
class CongestionCongrollerInterface
{
    public:
    CongestionCongrollerInterface(){};
    virtual ~CongestionCongrollerInterface() {};
    virtual void feed_packet(double send_time_ms, double arrival_time_ms, double current_incoming_bitrate)=0;
    virtual double get_target_bitrate() =0;
};
}
#endif


