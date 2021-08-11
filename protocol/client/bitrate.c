#include "bitrate.h"
volatile int max_bitrate = STARTING_BITRATE;
volatile int max_burst_bitrate = STARTING_BURST_BITRATE;
#define BAD_BITRATE 10400000
#define BAD_BURST_BITRATE 31800000

void fallback_bitrate(int num_nacks_per_second) {
    // our first dumb algorithm:
    // if we're nacking a lot, default to the old 10mbps/30mbps
    // and if we're on the old bad bitrate and not nacking, keep adding mbps until we hit the 16mbps
    // threshold
    if (num_nacks_per_second > 6 && max_bitrate != BAD_BITRATE) {
        max_bitrate = BAD_BITRATE;
        max_burst_bitrate = BAD_BURST_BITRATE;
    } else {
        if (max_bitrate < STARTING_BITRATE) {
            // add 1 mb (approximately)
            max_bitrate = min(max_bitrate + 1000000, STARTING_BITRATE);
            if (max_bitrate == STARTING_BITRATE) {
                max_burst_bitrate = STARTING_BURST_BITRATE;
            }
        }
    }
}

bool calculate_new_bitrate(int num_nacks_per_second) {
    int old_bitrate = max_bitrate;
    fallback_bitrate(num_nacks_per_second);
    return old_bitrate != max_bitrate;
}
