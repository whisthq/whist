#include <map>
#include <set>
#include <string>
using namespace std;

extern "C" {
#include "audio.h"
#include "audio_path.h"
#include <whist/utils/threads.h>
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

typedef double timestamp_ms;

// the interval of sending audio packet, keep it same as sender side
// TODO: calculate from SAMPLES_PER_FRAME
const int audio_packets_interval_ms = 10;

// how many frames/packets allowed to queue inside the audio device queue
// TODO: make this adaptive to reduce latency for good environment and reduce pop for though environment
const int max_num_inside_device_queue = 8;
// how many frames/packets allowed to queue inside the user queue (when device queue is full)
const int max_num_inside_user_queue = 20;
// how many frames/packets allowed to queue in total
const int max_total_queue_len = max_num_inside_user_queue + max_num_inside_device_queue;
// the target total_queue_len used by queue len managing
const int target_total_queue_len = max_num_inside_device_queue;

// enable verbose_log for debuging
const int verbose_log = 1;

// frame/packet with data and receive time
struct PacketInfo {
    string data;
    timestamp_ms receive_time;
    PacketInfo(unsigned char *buf, int size, timestamp_ms &t) : data(buf, buf + size) {
        receive_time = t;
    }
};

// the operations for dynamic queue len management
enum ManangeOperation { EarlyDrop = 0, EarlyDup = 1, NoOp = 2 };

/*
============================
Private Globals
============================
*/

// AudioContext used for access audio device
static AudioContext *g_audio_context = 0;
// the mutex to protect accessing the data structures
static WhistMutex g_mutex;
static WhistTimer g_timer;

// the cache value of device_queue_len
// the value is cached so that we don't need to use lock while re-init audio device
static atomic_int cached_device_queue_len;
// the context of last packt
static string last_packet_data;

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
static set<int> recent_popped_ids;
// the max id we have ever seen
static int max_seen_id = -1;

/*
----------------------------------
core data struct of use space queue
----------------------------------
*/
// this is the userspace audio queue, we use an ordered map to represnet a queue,
// allow insert at any position, but pop must be in order
static map<int, PacketInfo> user_queue;

/*
============================
Private Function Declarations
============================
*/

static double get_timestamp_ms();

static int multi_threaded_audio_renderer(void *);

// a dedicated thread for audio render
static int multi_threaded_audio_renderer(void *);

static int detect_skip_num(int user_queue_len, int device_queue_len);

static void pop_inner(unsigned char *buf, int *size);

static bool ready_to_pop(timestamp_ms now);

static ManangeOperation decide_queue_len_manage_operation(int user_queue_len, int device_queue_len,
                                                          timestamp_ms now);

/*
============================
Public Function Implementations
============================
*/

int audio_path_init(void) {
    start_timer(&g_timer);

    g_mutex = whist_create_mutex();
    g_audio_context = init_audio();

    atomic_init(&cached_device_queue_len, 0);
    whist_create_thread(multi_threaded_audio_renderer, "MultiThreadedAudioRenderer", NULL);

    return 0;
}

int push_to_audio_path(int id, unsigned char *buf, int size) {
    const int max_reordered_allowed_to_play = 5;
    // size of the anti_replay buffer
    const int anti_replay_window_size = 20;
    // a buffer to filter out duplicated frames/packets
    static set<int> anti_replay;

    int device_queue_len = atomic_load(&cached_device_queue_len);
    if (device_queue_len < 0) {
        return -1;
    }

    timestamp_ms now = get_timestamp_ms();
    PacketInfo packet_info(buf, size, now);

    whist_lock_mutex(g_mutex);

    // TODO better handling of reorder
    // when serious reoroder is detected, we can dely sending data to the device
    // so that the audio queue can put packets into correct order

    int anti_replay_window_lower_bound =
        anti_replay.empty() ? -1 : *anti_replay.begin() - anti_replay_window_size;

    if (id <= anti_replay_window_lower_bound ||
        anti_replay.find(id) != anti_replay.end())  // id too stale or already have this id
    {
        whist_unlock_mutex(g_mutex);
        return -1;
    }

    int max_poped_id = recent_popped_ids.empty() ? -1 : *recent_popped_ids.rbegin();

    if (id + max_reordered_allowed_to_play <= max_poped_id) {
        whist_unlock_mutex(g_mutex);
        return -1;
    }

    anti_replay.insert(id);
    while (anti_replay.size() > anti_replay_window_size) {
        anti_replay.erase(anti_replay.begin());
    }

    user_queue.emplace(id, packet_info);

    int user_queue_len = (int)user_queue.size();
    int total_queue_len = user_queue_len + device_queue_len;

    int expected_skip = detect_skip_num(user_queue_len, device_queue_len);

    if (expected_skip > 0) {
        if (verbose_log) {
            fprintf(stderr, "queue size=%d %d %d, has to skip %d!!\n", total_queue_len,
                    user_queue_len, device_queue_len, expected_skip);
        }

        for (int i = 0; i < expected_skip; i++) {
            // make sure we never erase buffered packets
            if ((int)user_queue.size() <= buffered_for_flush_cnt) {
                if (verbose_log) {
                    fprintf(stderr, "this usually shouldn't happen %d %d %d %d\n", (int)user_queue.size(),
                            buffered_for_flush_cnt, i, expected_skip);
                }
                break;
            }
            assert(!user_queue.empty());
            user_queue.erase(user_queue.begin());
        }
    }

    whist_unlock_mutex(g_mutex);

    return 0;
}

int pop_from_audio_path(unsigned char *buf, int *size) {
    int ret = -1;

    int device_queue_byte = get_device_audio_queue_bytes(g_audio_context);
    int device_queue_len = -1;
    if (device_queue_byte >= 0) {
        device_queue_len =
            (device_queue_byte + DECODED_BYTES_PER_FRAME - 1) / DECODED_BYTES_PER_FRAME;
    }

    timestamp_ms now = get_timestamp_ms();

    atomic_store(&cached_device_queue_len, device_queue_len);

    if (verbose_log && device_queue_byte == 0) {
        static timestamp_ms last_log_time = 0;
        if (now - last_log_time > 500) {
            last_log_time = now;
            fprintf(stderr, "buffer becomes empty!!!!\n");
        }
    }

    if (verbose_log) {
        static timestamp_ms last_log_time = 0;
        if (now - last_log_time > 100) {
            last_log_time = now;
            fprintf(stderr, "%d %d %d\n", (int)user_queue.size(), device_queue_len, device_queue_byte);
        }
    }

    whist_lock_mutex(g_mutex);

    int user_queue_len = (int)user_queue.size();

    if (device_queue_byte < 0) {
        user_queue.clear();  // if audio device is not ready drop all packets
        buffered_for_flush_cnt = 0;
        flushing_buffered_packets = 0;
        whist_unlock_mutex(g_mutex);
        return -1;
    }

    // unconditionally flush packets
    if (flushing_buffered_packets) {
        assert((int)user_queue.size() >= buffered_for_flush_cnt);
        pop_inner(buf, size);
        buffered_for_flush_cnt--;
        if (buffered_for_flush_cnt == 0) flushing_buffered_packets = false;
        whist_unlock_mutex(g_mutex);
        return 0;
    }

    // when ever device buffer reaches 0, we start to queue packet for anti-jitter
    // and flush the queued packet activately later
    if (device_queue_byte == 0) {
        if (user_queue.size() < max_num_inside_device_queue) {
            buffered_for_flush_cnt = (int)user_queue.size();
            whist_unlock_mutex(g_mutex);  // wait for more packets
            return -2;
        } else  // we have queued enough
        {
            pop_inner(buf, size);

            buffered_for_flush_cnt = max_num_inside_device_queue - 1;
            flushing_buffered_packets = true;

            whist_unlock_mutex(g_mutex);
            return 0;
        }
    } else {
        auto op = decide_queue_len_manage_operation(user_queue_len, device_queue_len, now);

        if (op == EarlyDup) {
            if (last_packet_data.length()) {
                memcpy(buf, last_packet_data.c_str(), last_packet_data.length());
                *size = (int)last_packet_data.length();
                whist_unlock_mutex(g_mutex);
                return 0;
            }
            // otherwise don't return continue running as normal
        } else if (op == EarlyDrop) {
            if (user_queue_len > 0) {
                int fake_size = 0;
                pop_inner(buf, &fake_size);  // drop this packet
            }
            // don't return after early drop, continue to run as normal
        }

        // if the device's buffer is  not avaliable for another packet
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
    assert(0 == 1);  // should not reach

    return ret;
}

/*
============================
Private Function Implementations
============================
*/

static double get_timestamp_ms()
{
    return get_timer(&g_timer)*MS_IN_SECOND;
}

static int multi_threaded_audio_renderer(void *) {
    while (1) {
        if (render_audio(g_audio_context) != 0) {
            whist_sleep(2);
        }
    }
    return 0;
}

static int detect_skip_num(int user_queue_len, int device_queue_len) {
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
        return skip_num;
    }

    return 0;
}

static void pop_inner(unsigned char *buf, int *size) {
    // how many recently popped ids we keep track
    // the capcity of recent_popped_ids below
    const int recent_popped_ids_capcity = 10;

    assert(!user_queue.empty());
    auto it = user_queue.begin();

    if (last_popped_id + 1 != it->first) {
        if (verbose_log) fprintf(stderr, "lost (or reordered) packet %d!!!\n", last_popped_id + 1);
    }

    last_popped_id = it->first;
    recent_popped_ids.insert(last_popped_id);
    while ((int)recent_popped_ids.size() > recent_popped_ids_capcity) {
        recent_popped_ids.erase(recent_popped_ids.begin());
    }

    memcpy(buf, it->second.data.c_str(), it->second.data.length());
    *size = (int)it->second.data.length();
    last_packet_data = it->second.data;
    user_queue.erase(it);
}

static bool ready_to_pop(timestamp_ms now) {
    const int anti_reorder_strength = 3;

    if (user_queue.empty()) return false;

    int current_packet_id = user_queue.begin()->first;
    timestamp_ms current_packet_receive_time = user_queue.begin()->second.receive_time;

    // if it's a consecutive packet
    if (recent_popped_ids.find(current_packet_id - 1) != recent_popped_ids.end()) {
        return true;
    }

    // if a packet has been stale for long, pop regardlessly
    if (now - current_packet_receive_time >= (anti_reorder_strength+0.5) * audio_packets_interval_ms) {
        if (verbose_log) {
            fprintf(stderr, "[popped %d by time staleness]\n", user_queue.begin()->first);
        }
        return true;
    }

    // if a packet with id + anti_reorder_strength has been seen, then we believe the packets
    // blocking the current packet has been lost
    if (current_packet_id + anti_reorder_strength <= max_seen_id) {
        if (verbose_log) {
            fprintf(stderr, "[popped %d by id staleness]\n", user_queue.begin()->first);
        }

        return true;
    }

    return false;
}

static ManangeOperation decide_queue_len_manage_operation(int user_queue_len, int device_queue_len,
                                                          timestamp_ms now) {
    const double queue_len_management_sensitivity = 1.5;
    const int target_sample_times = 10;
    const int sample_period = 100;
    // keep each value instead of running sum for easy debugging
    static int sampled_queue_lens[target_sample_times + 1];
    static int current_sample_cnt = 0;
    static timestamp_ms last_sample_time = 0;

    FATAL_ASSERT(device_queue_len > 0);
    // when calling this function, device_queue_len should never be <=zero,
    // guarenteed by upper level logic

    if (buffered_for_flush_cnt > 0) {
        current_sample_cnt = 0;
        return NoOp;
    }

    int total_len = user_queue_len + device_queue_len;

    if (now >= last_sample_time + sample_period) {
        sampled_queue_lens[current_sample_cnt] = total_len;
        current_sample_cnt++;
        last_sample_time = now;
    }

    FATAL_ASSERT(current_sample_cnt <= target_sample_times);

    if (current_sample_cnt < target_sample_times) {
        return NoOp;
    }

    // otherwise we have  current_sample_cnt =target_sample_times;

    double sum = 0;
    for (int i = 0; i < target_sample_times; i++) {
        sum += sampled_queue_lens[i];
    }
    double avg_len = sum / target_sample_times;

    // no matter which case, begin next sample period
    current_sample_cnt = 0;

    ManangeOperation op = NoOp;
    if (avg_len >= target_total_queue_len + queue_len_management_sensitivity) {
        if (verbose_log) {
            fprintf(stderr, "aduio_queue running high, len=%.2f %d %d %d, drop one frame!! ",
                    avg_len, total_len, user_queue_len, device_queue_len);
        }
        op = EarlyDrop;
    } else if (avg_len <= target_total_queue_len - queue_len_management_sensitivity) {
        if (verbose_log) {
            fprintf(stderr, "aduio_queue running low, len=%.2f %d %d %d, fill with last frame!! ",
                    avg_len, total_len, user_queue_len, device_queue_len);
        }
        op = EarlyDup;
    }

    if (verbose_log && op != NoOp) {
        fprintf(stderr, "last %d sampled length=[", target_sample_times);
        for (int i = 0; i < target_sample_times; i++) {
            fprintf(stderr, "%d,", sampled_queue_lens[i]);
        }
        fprintf(stderr, "]\n");
    }

    return op;
}
