#include <fractal/core/fractal.h>
#include <fractal/utils/threads.h>

#include "metrics.h"

FractalMutex tracking_mutex;

int packet_ids[MAX_NUM_TRACKED_PACKETS];
clock packet_timers[MAX_NUM_TRACKED_PACKETS];

void init_tracking() {
    tracking_mutex = fractal_create_mutex();
    for (int i = 0; i < MAX_NUM_TRACKED_PACKETS; ++i) {
        packet_ids[i] = -1;
    }
}

void destroy_tracking() { fractal_destroy_mutex(tracking_mutex); }

int find_index_for_id(int id) {
    for (int i = 0; i < MAX_NUM_TRACKED_PACKETS; ++i) {
        if (packet_ids[i] == id) {
            return i;
        }
    }
    return -1;
}

void track_packet(int id) {
    fractal_lock_mutex(tracking_mutex);
    int next_index = find_index_for_id(-1);
    if (next_index >= 0) {
        packet_ids[next_index] = id;
        start_timer(&packet_timers[next_index]);
    } else {
        LOG_INFO("Dropping packet %d, not enough space.", id);
    }
    fractal_unlock_mutex(tracking_mutex);
}

int track_new_packet() {
    static int next_packet_id = 0;
    track_packet(next_packet_id);
    return next_packet_id++;
}

void finish_tracking_packet(int id) {
    fractal_lock_mutex(tracking_mutex);
    int index = find_index_for_id(id);
    if (index >= 0) {
        float packet_time = get_timer(packet_timers[index]);
        packet_ids[index] = -1;
        LOG_INFO("Metrics | Packet %d | Latency | %f", id, packet_time);
    }
    fractal_unlock_mutex(tracking_mutex);
}

int *flush_tracked_packets() {
    int *ret = malloc(sizeof(packet_ids));
    fractal_lock_mutex(tracking_mutex);
    memcpy(ret, packet_ids, sizeof(packet_ids));
    fractal_unlock_mutex(tracking_mutex);
    for (int i = 0; i < MAX_NUM_TRACKED_PACKETS; ++i) {
        packet_ids[i] = -1;
    }
    return ret;
}
