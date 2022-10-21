#include "cc_shared_state.h"

#include "cc_shared_state.h"
#include "rtc_base/checks.h"
#include <vector>

CCSharedState cc_shared_state;
struct Point
{
    double x;
    double y;
};

static std::vector<Point> k_down_points= { {0.85,1}, {0.8,5}, {0.7,10}, {0.6,20}, {0.5,30}, {0.45,40}, {0.4,50}, {0.35,60}, {0.3,100} };

double get_linear_interpolate(double x, const std::vector<Point> & points)
{
    int n= points.size();
    RTC_CHECK(n>=2);
    if(x >= points[0].x ){
        return points[0].y;
    }
    if(x <= points[n-1].x){
        return points[n-1].y;
    }
    for(int i=0;i<n-1;i++)
    {
        if( x <= points[i].x && x >= points[i+1].x)
        {
            return points[i].y  +   (points[i].x -x)/ (points[i].x- points[i+1].x) * (points[i+1].y -points[i].y);
        }
    }
    //return -1;
    RTC_CHECK_NOTREACHED();
}

struct UnitTest
{
    UnitTest()
    {
        for(double i=0;i<1.0;i+=0.01)
        {
            printf("<%f,%f>",i, get_linear_interpolate(i, k_down_points));
        }
        //exit(0);
    }
};

//UnitTest test;

double CCSharedState::get_kdown()
{
        const double k_down_default=0.039;
        if(current_bitrate.IsInfinite()) return k_down_default/10;
        RTC_CHECK(max_bitrate.IsFinite());
        RTC_CHECK(min_bitrate.IsFinite());

        double bitrate_ratio= current_bitrate.bps()*1.0/ max_bitrate.bps();
        return k_down_default/get_linear_interpolate(bitrate_ratio, k_down_points);
};
