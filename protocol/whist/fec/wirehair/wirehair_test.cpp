#include <vector>
#include <random>
#include <tuple>

#include "whist/core/whist.h"
#include "whist/logging/logging.h"
extern "C" {
#include "whist/utils/clock.h"
#include <whist/fec/wirehair/wirehair.h>
};

#include <string.h>
#include <assert.h>
using namespace std;

extern "C" {
#include "wirehair_test.h"
};

static const int g_use_shuffle = 1;
static const int g_codec_reuse = 0;

static int base_log = 0;
static int verbose_log = 0;

static double get_cputime_ms(void) {
#if OS_IS(OS_MACOS) || OS_IS(OS_LINUX)
    struct timespec t2;
    // use CLOCK_MONOTONIC for relative time
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);

    double elapsed_time = (t2.tv_sec) * MS_IN_SECOND;
    elapsed_time += (t2.tv_nsec) / (double)NS_IN_MS;

    double ret = elapsed_time;
    return ret;
#else
    // for other operation system fallback to noraml time
    return get_timestamp_sec() * MS_IN_SECOND;
#endif
}

static int fast_rand(void) {
    static unsigned int g_seed = 1111;
    g_seed = (214013 * g_seed + 2531011);
    return (g_seed >> 16) & 0x7FFF;
}

static int better_rand() {
    static random_device rd;
    static mt19937 g(rd());
    static uniform_int_distribution<> dis(1, 1000 * 1000 * 1000);
    return dis(g);
}

static vector<char *> make_buffers(int num, int segment_size) {
    vector<char *> buffers(num);
    for (int i = 0; i < num; i++) {
        buffers[i] = (char *)malloc(segment_size);
    }
    return buffers;
}

static void free_buffers(vector<char *> buffers) {
    for (int i = 0; i < (int)buffers.size(); i++) {
        free(buffers[i]);
    }
}
static void split_copy(string &block, char *output[], int segment_size) {
    assert(block.size() % segment_size == 0);
    int num = (int)block.size() / segment_size;
    for (int i = 0; i < num; i++) {
        memcpy(output[i], &block[i * segment_size], segment_size);
    }
}

[[maybe_unused]] static string combine_copy(vector<char *> buffers, int segment_size) {
    string res;
    for (int i = 0; i < (int)buffers.size(); i++) {
        string tmp(segment_size, 'a');
        memcpy(&tmp[0], buffers[i], segment_size);
        res += tmp;
    }
    return res;
}

std::tuple<int, double, double> one_test(int segment_size, int num_real, int num_fec) {
    string input(segment_size * num_real, 'a');

    for (int i = 0; i < (int)input.size(); i++) {
        input[i] = fast_rand() % 256;
    }

    auto buffers = make_buffers(num_real + num_fec, segment_size);
    split_copy(input, &buffers[0], segment_size);

    double encode_time_total = 0;

    WirehairCodec wirehair_encoder, dummy_old_encoder = nullptr;

    if (g_codec_reuse) {
        dummy_old_encoder =
            wirehair_encoder_create(nullptr, &input[0], segment_size * num_real, segment_size);
    }

    {
        double t1 = get_cputime_ms();
        wirehair_encoder = wirehair_encoder_create(dummy_old_encoder, &input[0],
                                                   segment_size * num_real, segment_size);
        double t2 = get_cputime_ms();
        if (base_log) {
            fprintf(stderr, "<encoder create: %.3f>", t2 - t1);
        }
        encode_time_total += t2 - t1;
    }

    double before_encode_loop_time = get_cputime_ms();

    double max_encode_iteration_time = 0;
    double min_encode_iteration_time = 9999;
    for (int i = 0; i < num_fec; i++) {
        int idx = num_real + i;
        uint32_t out_size;

        double t1 = get_cputime_ms();
        int r = wirehair_encode(wirehair_encoder, idx, buffers[idx], segment_size, &out_size);
        double t2 = get_cputime_ms();

        if (verbose_log) {
            fprintf(stderr, "<encode idx=%d: %.3f>", idx, t2 - t1);
        }

        max_encode_iteration_time = max(t2 - t1, max_encode_iteration_time);
        min_encode_iteration_time = min(t2 - t1, min_encode_iteration_time);

        FATAL_ASSERT(r == 0);
        FATAL_ASSERT(segment_size == (int)out_size);
    }

    double encode_loop_time = get_cputime_ms() - before_encode_loop_time;

    if (base_log) {
        fprintf(stderr, "<encode loop: %3f>", encode_loop_time);
        fprintf(stderr, "<min/max/avg encode iteration: %.3f/%.3f/%.3f>", min_encode_iteration_time,
                max_encode_iteration_time, encode_loop_time / num_fec);
    }

    encode_time_total += encode_loop_time;

    vector<int> shuffle;
    for (int i = 0; i < num_real + num_fec; i++) {
        shuffle.push_back(i);
    }

    for (int i = 0; i < (int)shuffle.size(); i++) {
        swap(shuffle[i], shuffle[better_rand() % shuffle.size()]);
    }

    string output(num_real * segment_size, 'b');
    WirehairCodec wirehair_decoder, dummy_old_decoder = nullptr;
    double decode_time_total = 0;

    if (g_codec_reuse) {
        dummy_old_decoder =
            wirehair_decoder_create(nullptr, (num_real + 1) * segment_size, segment_size);
    }

    {
        double t1 = get_cputime_ms();
        wirehair_decoder =
            wirehair_decoder_create(dummy_old_decoder, num_real * segment_size, segment_size);
        double t2 = get_cputime_ms();
        decode_time_total += t2 - t1;
        if (base_log) {
            fprintf(stderr, "<decoder create: %.3f>", t2 - t1);
        }
    }

    int succ = 0;
    int decode_packet_cnt = 0;
    double before_decode_loop_time = get_cputime_ms();

    double max_decode_iteration_time = 0;
    double min_decode_iteration_time = 9999;
    double last_decode_iteration_time = -1;
    for (int i = num_fec + num_real - 1; i >= 0; i--) {
        int idx;
        if (g_use_shuffle) {
            idx = shuffle[i];
        } else {
            idx = i;
        }
        decode_packet_cnt++;
        double t1 = get_cputime_ms();
        int r = wirehair_decode(wirehair_decoder, idx, buffers[idx], segment_size);
        double t2 = get_cputime_ms();
        if (verbose_log) {
            fprintf(stderr, "<decode feed idx=%d: %.3f>", idx, t2 - t1);
        }

        if (r != 0) {
            max_decode_iteration_time = max(t2 - t1, max_decode_iteration_time);
            min_decode_iteration_time = min(t2 - t1, min_decode_iteration_time);
        } else {
            last_decode_iteration_time = t2 - t1;
        }

        if (r == 0) {
            succ = 1;
            break;
        }
    }
    double decode_loop_time = get_cputime_ms() - before_decode_loop_time;
    decode_time_total += decode_loop_time;
    if (base_log) {
        fprintf(stderr, "<decode loop: %.3f>", decode_loop_time);
        fprintf(stderr, "<min/max/avg decode iteration: %.3f/%.3f/%.3f>", min_decode_iteration_time,
                max_decode_iteration_time, decode_loop_time / decode_packet_cnt);
        fprintf(stderr, "<last decode iteration: %.3f>", last_decode_iteration_time);
    }

    if (succ != 1) {
        return make_tuple(-1, 0.0, 0.0);
    }

    {
        double t1 = get_cputime_ms();
        FATAL_ASSERT(wirehair_recover(wirehair_decoder, &output[0], num_real * segment_size) == 0);
        double t2 = get_cputime_ms();
        if (base_log) {
            fprintf(stderr, "<decode recover: %.3f>", t2 - t1);
        }
        decode_time_total += t2 - t1;
    }

    FATAL_ASSERT(input.size() == output.size());
    if (input != output) {
        return make_tuple(-2, 0.0, 0.0);
    }

    free_buffers(buffers);
    wirehair_free(wirehair_encoder);
    wirehair_free(wirehair_decoder);

    return make_tuple(decode_packet_cnt - num_real, encode_time_total, decode_time_total);
}

void overhead_test(void) {
    int round = 10000;
    int segment_size = 4;

    vector<int> num_fec_packets = {1, 2, 5, 10, 20, 50, 100, 200, 500};
    for (int i = 2; i < 256; i++) {
        for (int ii = 0; ii < 3; ii++) {
            fprintf(stderr, "real=%d; ", i);
            for (int j = 0; j < (int)num_fec_packets.size(); j++) {
                int o1_cnt = 0, o2_cnt = 0, o3_cnt = 0;
                int o5_cnt = 0, o10_cnt = 0;
                int o_sum = 0;
                int y = num_fec_packets[j];
                int max_overhead = 0;
                for (int k = 0; k < round; k++) {
                    auto [overhead, encode_time, decode_time] = one_test(segment_size, i, y);
                    FATAL_ASSERT(overhead >= 0);
                    o_sum += overhead;
                    if (overhead >= 1) o1_cnt++;
                    if (overhead >= 2) o2_cnt++;
                    if (overhead >= 3) o3_cnt++;
                    if (overhead >= 5) o5_cnt++;
                    if (overhead >= 10) o10_cnt++;
                    if (overhead > max_overhead) {
                        max_overhead = overhead;
                    }
                }
                fprintf(stderr, "<%d;%3d,%3d,%2d,%2d,%2d;%3d;%3d>   ", y, o1_cnt, o2_cnt, o3_cnt,
                        o5_cnt, o10_cnt, max_overhead, o_sum);
            }
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    }
}

void performance_test(void) {
    int round = 3000;
    int segment_size = 4;

    vector<int> num_fec_packets = {1, 2, 5, 10, 20, 50, 100, 200, 500};
    for (int i = 2; i < 1024; i++) {
        for (int ii = 0; ii < 3; ii++) {
            fprintf(stderr, "real=%d; ", i);
            for (int j = 0; j < (int)num_fec_packets.size(); j++) {
                int y = num_fec_packets[j];

                double encode_time_min = 9999;
                double encode_time_max = 0;
                double encode_time_sum = 0;

                double decode_time_min = 9999;
                double decode_time_max = 0;
                double decode_time_sum = 0;

                for (int k = 0; k < round; k++) {
                    auto [overhead, encode_time, decode_time] = one_test(segment_size, i, y);
                    FATAL_ASSERT(overhead >= 0);
                    encode_time_min = min(encode_time_min, encode_time);
                    encode_time_max = max(encode_time_max, encode_time);
                    encode_time_sum += encode_time;

                    decode_time_min = min(decode_time_min, decode_time);
                    decode_time_max = max(decode_time_max, decode_time);
                    decode_time_sum += decode_time;
                }

                fprintf(stderr, "<%d; %.0f;%.0f,%.0f; %.0f,%.0f,%.0f>   ", y,
                        encode_time_min * 1000, encode_time_max * 1000,
                        encode_time_sum / round * 1000, decode_time_min * 1000,
                        decode_time_max * 1000, decode_time_sum / round * 1000);
            }
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    }
}

void performance_of_phases(void) {
    int segment_size = 1280;
    vector<int> num_real_packets = {2,   5,    10,   20,   50,    100,   200,
                                    500, 1000, 2000, 5000, 10000, 20000, 50000};
    vector<int> num_fec_packets = {1,   2,    5,    10,   20,    100,   200,
                                   500, 1000, 2000, 5000, 10000, 20000, 50000};

    base_log = 1;
    for (int i = 0; i < (int)num_real_packets.size(); i++) {
        int x = num_real_packets[i];
        for (int j = 0; j < (int)num_fec_packets.size(); j++) {
            int y = num_fec_packets[j];
            fprintf(stderr, "num_real=%5d num_fec=%5d ", x, y);
            auto [overhead, encode_time, decode_time] = one_test(segment_size, x, y);
            FATAL_ASSERT(overhead >= 0);
            fprintf(stderr, " encode_total=%f decode_total=%f", encode_time, decode_time);
            fprintf(stderr, "\n");
        }
    }
    base_log = 0;
}
int wirehair_auto_test(void) {
    wirehair_init();

    int segment_size = 1280;
    int round = 1000;

    for (int i = 0; i < round; i++) {
        int num_real = better_rand() % 1024 + 2;
        int num_fec = better_rand() % 1024;
        auto [overhead, encode_time, decode_time] = one_test(segment_size, num_real, num_fec);

        if (overhead < 0) {
            fprintf(stderr, "error %d with num_real=%d num_fec=%d\n", overhead, num_real, num_fec);
            return overhead;
        }

        if (verbose_log) {
            fprintf(stderr,
                    "<num_real=%d, num_fec=%d, encode_time=%f,decode_time=%f, overhead=%d>\n",
                    num_real, num_fec, encode_time, decode_time, overhead);
        }
    }

    return 0;
}

int wirehair_manual_test(void) {
    whist_set_thread_priority(WHIST_THREAD_PRIORITY_REALTIME);

    wirehair_init();

    get_timestamp_sec();

    // performance_test();

    // overhead_test();

    // performance_of_phases();

    return 0;
}
