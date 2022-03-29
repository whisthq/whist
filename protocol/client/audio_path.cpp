/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file audio_path.c
 * @brief This file contains an implement of data path from network to audio device
 */

#include <map>
#include <set>
#include <string>
using namespace std;

extern "C" {
#include "audio.h"
#include "audio_path.h"
#include <whist/utils/clock.h>
#include <whist/logging/logging.h>
};

#include "whist/utils/atomic.h"
#include "string.h"
#include "assert.h"

/*
============================
Defines
============================
*/

typedef double timestamp_ms;  // NOLINT

// the interval of sending audio packet, keep it same as sender side
// TODO: calculate from SAMPLES_PER_FRAME
const int audio_packets_interval_ms = 10;

// how many frames/packets allowed to queue inside the audio device queue
// TODO: make this adaptive to reduce latency for good environment and reduce pop for tough
// environment
const int max_num_inside_device_queue = 8;
// how many frames/packets allowed to queue inside the user queue (when device queue is full)
const int max_num_inside_user_queue = 12;
// how many frames/packets allowed to queue in total
const int max_total_queue_len = max_num_inside_user_queue + max_num_inside_device_queue;
// the target total_queue_len used by queue len managing
const int target_total_queue_len = max_num_inside_device_queue;

// toggle verbose_log for debuging
const int verbose_log = 0;

// frame/packet with data and receive time
struct PacketInfo {
    string data;
    timestamp_ms receive_time;
    PacketInfo(unsigned char *buf, int size, timestamp_ms &t) : data(buf, buf + size) {
        receive_time = t;
    }
    PacketInfo(PacketInfo &&other) : data(move(other.data)) { receive_time = other.receive_time; }
};

// the operations for dynamic queue len management
enum ManangeOperation { EARLY_DROP = 0, EARLY_DUP = 1, NO_OP = 2 };

/*
============================
Private Globals
============================
*/

// the mutex to protect accessing the data structures
static WhistMutex g_mutex;
static WhistTimer g_timer;

// the cache value of device_queue_len
// the value is cached so that we don't need to use lock while re-init audio device
static atomic_int cached_device_queue_len;

// the expected size of a decod frame, inited by init function, used by pop function
static int decoded_bytes_per_frame = -1;

/*
----------------------------------
buffering packet related
----------------------------------
*/
// how many packets are buffered for flush
static int buffered_for_flush_cnt = 0;
// if this value is true, we are in the middle of flushing buffered packets
static bool flushing_buffered_packets = 0;

/*
----------------------------------
for tracking recent packets
----------------------------------
*/
// last id popped to decoder
static int last_popped_id = -1;
// a set of recent popped id
static set<int> *recent_popped_ids;
// the max id we have ever seen
static int max_seen_id = -1;
// the content of last poped packt
static string last_popped_packet_data;
/*
----------------------------------
core data struct of use space queue
----------------------------------
*/
// this is the userspace audio queue, we use an ordered map to represnet a queue,
// allow insert at any position, but pop must be in order
static map<int, PacketInfo> *user_queue;

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          internal monotonic timestamp function for audio path
 *
 * @returns                        monotonic timestamp in ms
 */
static double get_timestamp_ms();

/**
 * @brief                          this is the decision logic for audio frame skip
 *
 * @param user_queue_len           num of frames queued inside user queue
 * @param device_queue_len         num of frames queued inside device queue
 * @returns                        the num of frames that need to be skipped
 */
static int detect_skip_num(int user_queue_len, int device_queue_len);

/**
 * @brief                          a helper function to pop a frame from the user queue
 *
 * @param buf                      a pointer to store the popped frame, should be allocated by
 *                                 called
 * @param size                     store the size of popped frame
 */
static void pop_inner(unsigned char *buf, int *size);

/**
 * @brief                          this is the decision logic of whehter the user queue is ready to
 * pop
 *
 * @param now                      the timestamps of the time calling
 * @returns                        true if ready
 *
 * @note                           the return value might be false even if user queue is not empty,
 *                                 for anti-reordering
 */
static bool ready_to_pop(timestamp_ms now);

/**
 * @brief                          this is the decision logic dynamic queue len management
 *
 * @param user_queue_len           num of frames queued inside user queue
 * @param device_queue_len         num of frames queued inside device queue
 * @param now                      the timestamps of the time calling
 * @returns                        the operation that needs to be peformed, might be drop, dup or
 *                                 no_op.
 *
 * @note                           the goal of dynamic queue len management is to avoid user_queue
 *                                 full or device_queue empty, to avoid serious bad effect in sound
 *                                 in advance
 */
static ManangeOperation decide_queue_len_manage_operation(int user_queue_len, int device_queue_len,
                                                          timestamp_ms now);
/*
============================
Public Function Implementations
============================
*/

int audio_path_init(int in_decoded_bytes_per_frame) {
    // init the timer for get_timestamp_ms()
    start_timer(&g_timer);
    // more initilization
    g_mutex = whist_create_mutex();
    atomic_init(&cached_device_queue_len, -1);

    // save the passed in decoded_bytes_per_frame
    decoded_bytes_per_frame = in_decoded_bytes_per_frame;

    // allocate the data strucutres
    recent_popped_ids = new set<int>;
    user_queue = new map<int, PacketInfo>;

    return 0;
}

int push_to_audio_path(int id, unsigned char *buf, int size) {
    // if a packet comes with an id  <= the max id has been sent to device by 5
    // then we consider this packet as too old to play, and drop it directly
    const int distant_too_old_to_play = 5;
    // a buffer to filter out duplicated frames/packets
    static set<int> *anti_replay = new set<int>;
    // size of the anti_replay buffer
    // no need to tune this value, just use a conservative large value
    const int anti_replay_window_size = 100;

    // get the cached device queue length
    int device_queue_len = atomic_load(&cached_device_queue_len);
    if (device_queue_len < 0) {
        return -1;
    }

    // get current timestamps for multiple usages
    timestamp_ms now = get_timestamp_ms();

    // store packet with it's receive time
    PacketInfo packet_info(buf, size, now);

    // detect if a packet is replayed, or it's below the anti-relay bound
    int anti_replay_window_lower_bound =
        anti_replay->empty() ? -1 : *anti_replay->begin() - anti_replay_window_size;
    if (id <= anti_replay_window_lower_bound ||
        anti_replay->find(id) != anti_replay->end())  // id too stale or already have this id
    {
        return -2;
    }
    // insert the id into anti-replay
    anti_replay->insert(id);
    while (anti_replay->size() > anti_replay_window_size) {
        anti_replay->erase(anti_replay->begin());
    }

    // now entering the section that needs to be protected
    whist_lock_mutex(g_mutex);

    // alias of the max popped id from recent_popped_ids
    int max_popped_id = recent_popped_ids->empty() ? -1 : *recent_popped_ids->rbegin();

    // detect if a packet is too old to play
    if (id + distant_too_old_to_play <= max_popped_id) {
        whist_unlock_mutex(g_mutex);
        return -3;
    }

    // save the data and info inside user queue
    user_queue->emplace(id, move(packet_info));

    // convenient alias
    int user_queue_len = (int)user_queue->size();
    int total_queue_len = user_queue_len + device_queue_len;

    // detect the num of packets that need to be skipped
    int expected_skip = detect_skip_num(user_queue_len, device_queue_len);

    // do packet skip
    if (expected_skip > 0) {
        if (verbose_log) {
            fprintf(stderr, "queue size=%d %d %d, has to skip %d!!\n", total_queue_len,
                    user_queue_len, device_queue_len, expected_skip);
        }

        LOG_INFO_RATE_LIMITED(5.0, 3,
                              "too many auido frames queued, has to skip %d. total_len=%d, "
                              "user_queue_len=%d, device_queue_len=%d",
                              expected_skip, total_queue_len, user_queue_len, device_queue_len);

        for (int i = 0; i < expected_skip; i++) {
            // make sure we never erase buffered packets
            if ((int)user_queue->size() <= buffered_for_flush_cnt) {
                if (verbose_log) {
                    fprintf(stderr, "this usually shouldn't happen %d %d %d %d\n",
                            (int)user_queue->size(), buffered_for_flush_cnt, i, expected_skip);
                }
                break;
            }
            FATAL_ASSERT(!user_queue->empty());
            user_queue->erase(user_queue->begin());
        }
    }

    // leave the protected section
    whist_unlock_mutex(g_mutex);

    return 0;
}

int pop_from_audio_path(unsigned char *buf, int *size, int device_queue_bytes) {
    FATAL_ASSERT(decoded_bytes_per_frame > 0);

    // a flag only for log purpose
    static bool device_queue_empty_log_printed = true;

    // the pedning queue len manage operation
    static ManangeOperation pending_op = NO_OP;

    // calculate device_queue_len based on devices_queue_bytes
    int device_queue_len = -1;
    if (device_queue_bytes >= 0) {
        device_queue_len =
            (device_queue_bytes + decoded_bytes_per_frame - 1) / decoded_bytes_per_frame;
    }
    // cache the value of device_queue_len
    atomic_store(&cached_device_queue_len, device_queue_len);

    // get current timestamps for multiple usages
    timestamp_ms now = get_timestamp_ms();

    // for debug
    if (verbose_log && device_queue_bytes == 0) {
        static timestamp_ms last_log_time = 0;
        if (now - last_log_time > 500) {
            last_log_time = now;
            fprintf(stderr, "buffer becomes empty!!!!\n");
        }
    }

    // enter the protected section
    whist_lock_mutex(g_mutex);

    // for debug
    if (verbose_log) {
        static timestamp_ms last_log_time = 0;
        if (now - last_log_time > 100) {
            last_log_time = now;
            fprintf(stderr, "%d %d %d\n", (int)user_queue->size(), device_queue_len,
                    device_queue_bytes);
        }
    }

    // for robustness, if audio device is for some reason not ready drop all packets
    if (device_queue_bytes < 0) {
        // clear all data and states
        user_queue->clear();
        buffered_for_flush_cnt = 0;
        flushing_buffered_packets = false;
        device_queue_empty_log_printed = false;
        pending_op = NO_OP;
        whist_unlock_mutex(g_mutex);
        return -1;
    }

    // if we are in the status of flushing_buffered_packets, unconditionally flush packets
    if (flushing_buffered_packets) {
        FATAL_ASSERT((int)user_queue->size() >= buffered_for_flush_cnt);
        if(!ready_to_pop(now)){
            whist_unlock_mutex(g_mutex);
            return -5;
        }
        pop_inner(buf, size);
        buffered_for_flush_cnt--;
        if (buffered_for_flush_cnt == 0) {
            // enter next life cycle of "normal" state
            flushing_buffered_packets = false;
            device_queue_empty_log_printed = false;
            pending_op = NO_OP;
        }
        whist_unlock_mutex(g_mutex);
        return 0;
    }

    // when ever device buffer reaches 0, noticable regression is unavoidabled
    // the best thing to do is to stop playing and start to queue packets imediately
    // we start to queue packet for anti-jitter and flush the queued packet activately later
    if (device_queue_bytes == 0) {
        if (!device_queue_empty_log_printed) {
            LOG_INFO(
                "audio device queue becomes empty, begin buffering frames. user_queue_len=%d\n",
                (int)user_queue->size());
            device_queue_empty_log_printed = true;
        }
        // the status of start buffering or on the way of buffering
        if (user_queue->size() < max_num_inside_device_queue) {
            buffered_for_flush_cnt = (int)user_queue->size();
            whist_unlock_mutex(g_mutex);  // wait for more packets
            return -2;
        } else  // we have buffered enough
        {
            LOG_INFO("bufferred enough frames for audio, buffered %d.\n", (int)user_queue->size());

            // indicdate the start of flushing
            buffered_for_flush_cnt = max_num_inside_device_queue;
            flushing_buffered_packets = true;

            // robustness check
            FATAL_ASSERT(max_num_inside_device_queue > 1);

            // flush one packet for current iteration
            pop_inner(buf, size);
            buffered_for_flush_cnt--;

            whist_unlock_mutex(g_mutex);
            return 0;
        }
    } else {  // otherwise the audio path is in a normal state

        FATAL_ASSERT(buffered_for_flush_cnt == 0);

        // if there is no operation pending
        if(pending_op == NO_OP) {
            // detect operation for dynamic queue len management
            pending_op = decide_queue_len_manage_operation((int)user_queue->size(), device_queue_len, now);
        }

        // if it's early drop, drop one packet inside user queue
        if (pending_op == EARLY_DROP) {
            if (ready_to_pop(now)) {
                int fake_size;
                // drop this packet by a dummy pop
                // upper level will not feel this pop
                pop_inner(buf, &fake_size);
                pending_op = NO_OP;
            }
            // don't return after early drop, continue to run as normal
        }
        // if it's EARLY_DUP, dup the last saved packet
        else if (pending_op == EARLY_DUP) {
            // make sure we have a last packet saved
            if (last_popped_packet_data.length() && device_queue_len < max_num_inside_device_queue) {
                memcpy(buf, last_popped_packet_data.c_str(), last_popped_packet_data.length());
                *size = (int)last_popped_packet_data.length();
                pending_op = NO_OP;
                whist_unlock_mutex(g_mutex);
                return 0;
            }
            // otherwise don't return continue running as normal
        }

        // if the device's buffer is not avaliable for another packet
        if (device_queue_len >= max_num_inside_device_queue) {
            whist_unlock_mutex(g_mutex);
            return -3;
        }

        // the user queue is ready to pop
        if (ready_to_pop(now)) {
            pop_inner(buf, size);
            whist_unlock_mutex(g_mutex);
            return 0;
        }

        whist_unlock_mutex(g_mutex);
        return -4;
    }
}

/*
============================
Private Function Implementations
============================
*/

static double get_timestamp_ms() { return get_timer(&g_timer) * MS_IN_SECOND; }

static int detect_skip_num(int user_queue_len, int device_queue_len) {

    // a flag to toggle between smooth skip and bunch skip
    const bool smooth_skip = true;

    if (device_queue_len < 0) {
        // drop everything if audio device is not initilized
        return user_queue_len;
    }

    int total_len = user_queue_len + device_queue_len;

    if (total_len >= max_total_queue_len) {
        // when we hit the hard limit we remove everything in excess,
        // to let the queue len go back to normal
        int skip_num = total_len - target_total_queue_len;
        if (skip_num > user_queue_len) {
            skip_num = user_queue_len;
        }
        if(smooth_skip && skip_num>0){
            return 1;
        }
        return skip_num;
    }

    return 0;
}

static void pop_inner(unsigned char *buf, int *size) {
    // how many recently popped ids we keep track
    // the capcity of recent_popped_ids below
    // no need to tune this value, just use a conservative large value
    const int recent_popped_ids_capcity = 100;

    // it's guarentteed by upper level, when pop_inner is called, there must be something inside
    // user queue to pop
    assert(!user_queue->empty());

    auto it = user_queue->begin();

    // log non-consecutive packets
    if (last_popped_id + 1 != it->first) {
        if (verbose_log)
            fprintf(stderr, "non-consecutive packet %d!!! last_popped_id=%d\n", it->first,
                    last_popped_id);
    }

    // keep track of last popped id
    last_popped_id = it->first;

    // keep track of a set of recently poped ids
    recent_popped_ids->insert(last_popped_id);
    // keep the size of recent_popped_ids <= recent_popped_ids_capcity
    while ((int)recent_popped_ids->size() > recent_popped_ids_capcity) {
        recent_popped_ids->erase(recent_popped_ids->begin());
    }

    // copy packet to output buffer
    memcpy(buf, it->second.data.c_str(), it->second.data.length());
    *size = (int)it->second.data.length();

    // remember the content of last popped packet, for packet dup
    last_popped_packet_data = it->second.data;
    user_queue->erase(it);
}

static bool ready_to_pop(timestamp_ms now) {
    // max "time" to wait for an empty slot in num of frames,
    // so that an empty slot is considered lost.
    // TODO: make this adaptive, it's going to be a decent improvement
    // TODO: after this is adaptive, consider making device queue len longer for large value here
    const int anti_reorder_strength = 3;

    // if nothing is inside user queue
    if (user_queue->empty()) return false;

    // alias for the packet at the beginning of the user queue
    int current_packet_id = user_queue->begin()->first;
    timestamp_ms current_packet_receive_time = user_queue->begin()->second.receive_time;

    // if it's a consecutive packet
    if (recent_popped_ids->find(current_packet_id - 1) != recent_popped_ids->end()) {
        return true;
    }
    // otherwise the current packet is blocked by an empty slot before that

    // if a packet has been stale for long, then we believe the packets of the empty slots blocking
    // the current packet has been lost.
    if (now - current_packet_receive_time >=
        (anti_reorder_strength - 1 + 0.5) * audio_packets_interval_ms) {
        if (verbose_log) {
            fprintf(stderr, "[popped %d by time staleness]\n", user_queue->begin()->first);
        }
        return true;
    }

    // if a packet with id + anti_reorder_strength has been seen, then we believe the packets
    // of the empty slots blocking the current packet has been lost.
    // note: the above stragety works better when there are too many packet losses, this strategy
    // works better when packets are queued and squeezed together. so it's better to have both.
    if (current_packet_id + anti_reorder_strength - 1 <= max_seen_id) {
        if (verbose_log) {
            fprintf(stderr, "[popped %d by id staleness]\n", user_queue->begin()->first);
        }

        return true;
    }

    return false;
}

static ManangeOperation decide_queue_len_manage_operation(int user_queue_len, int device_queue_len,
                                                          timestamp_ms now) {
    // the sensitivity of dynamic queue_len management
    // queue_len management is trigger when the average total_len is
    // outside of target_total_queue_len ± queue_len_management_sensitivity
    const double queue_len_management_sensitivity = 1.2;
    // how many samples need to be sampled to calculate average queue len
    const int target_sample_times = 10;
    // period between sampling, in ms
    const int sample_period = 100;
    // keep each sample value instead of running sum for easy debugging
    static int sampled_queue_lens[target_sample_times + 1];
    // how many samples have been collected for the current sample attempt
    static int current_sample_cnt = 0;
    // the last time of sampling
    static timestamp_ms last_sample_time = 0;

    // when calling this function, device_queue_len should never be <=zero,
    // guarenteed by upper level logic
    FATAL_ASSERT(device_queue_len > 0);

    // when calling this function, buffered_for_flush_cnt should always be 0,
    // guarenteed by upper level logic
    FATAL_ASSERT(buffered_for_flush_cnt ==0 );

    // the total len of queue
    int total_len = user_queue_len + device_queue_len;

    // do one sample
    if (now >= last_sample_time + sample_period) {
        sampled_queue_lens[current_sample_cnt] = total_len;
        current_sample_cnt++;
        last_sample_time = now;
    }

    FATAL_ASSERT(current_sample_cnt <= target_sample_times);

    // in the middling of sampling for `target_sample_times` packets
    if (current_sample_cnt < target_sample_times) {
        return NO_OP;
    }
    // otherwise we have  current_sample_cnt =target_sample_times;

    // calculated averaget queue len
    double sum = 0;
    for (int i = 0; i < target_sample_times; i++) {
        sum += sampled_queue_lens[i];
    }
    double avg_len = sum / target_sample_times;

    ManangeOperation op = NO_OP;
    // if the queue is running high, do early drop to make it lower
    if (avg_len >= target_total_queue_len + queue_len_management_sensitivity) {
        if (verbose_log) {
            fprintf(stderr, "aduio_queue running high, len=%.2f, %d %d %d, drop one frame!! ",
                    avg_len, total_len, user_queue_len, device_queue_len);
        }
        op = EARLY_DROP;
    }  // if the queue is running low, do early dup to make it higher
    else if (avg_len <= target_total_queue_len - queue_len_management_sensitivity) {
        if (verbose_log) {
            fprintf(stderr, "aduio_queue running low, len=%.2f, %d %d %d, fill with last frame!! ",
                    avg_len, total_len, user_queue_len, device_queue_len);
        }
        op = EARLY_DUP;
    }

    // print out the sampled queue len for easy debugging
    if (verbose_log && op != NO_OP) {
        fprintf(stderr, "last %d sampled length=[", target_sample_times);
        for (int i = 0; i < target_sample_times; i++) {
            fprintf(stderr, "%d,", sampled_queue_lens[i]);
        }
        fprintf(stderr, "]\n");
    }

    // no matter which case, begin next sample period
    current_sample_cnt = 0;

    return op;
}
