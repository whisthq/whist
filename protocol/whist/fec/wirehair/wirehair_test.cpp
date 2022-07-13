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
static int base_log = 0;
static int verbose_log = 0;

inline double get_timestamp_ms0(void) { return get_timestamp_sec() * 1000; }

static double get_cputime_ms(void) {
#if OS_IS(OS_MACOS) || OS_IS(OS_MACOS)
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

int my_rand() {
    static random_device rd;
    static mt19937 g(rd());
    static uniform_int_distribution<> dis(1, 1000 * 1000 * 1000);
    return dis(g);
}

vector<char *> make_buffers(int num, int segment_size) {
    vector<char *> buffers(num);
    for (int i = 0; i < num; i++) {
        buffers[i] = (char *)malloc(segment_size);
    }
    return buffers;
}

void free_buffers(vector<char *> buffers) {
    for (int i = 0; i < (int)buffers.size(); i++) {
        free(buffers[i]);
    }
}
void split_copy(string &block, char *output[], int segment_size) {
    assert(block.size() % segment_size == 0);
    int num = (int)block.size() / segment_size;
    for (int i = 0; i < num; i++) {
        memcpy(output[i], &block[i * segment_size], segment_size);
    }
}

string combine_copy(vector<char *> buffers, int segment_size) {
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
        input[i] = my_rand() % 256;
    }

    auto buffers = make_buffers(num_real + num_fec, segment_size);
    split_copy(input, &buffers[0], segment_size);

    double encode_time_total = 0;

    WirehairCodec wirehair_encoder;

    {
        double t1 = get_cputime_ms();
        wirehair_encoder =
            wirehair_encoder_create(nullptr, &input[0], segment_size * num_real, segment_size);
        double t2 = get_cputime_ms();
        if (base_log) {
            fprintf(stderr, "<encoder create: %.3f>", t2 - t1);
        }
        encode_time_total += t2 - t1;
    }

    double before_encode_loop_time = get_cputime_ms();

    for (int i = 0; i < num_fec; i++) {
        int idx = num_real + i;
        uint32_t out_size;

        double t1 = get_cputime_ms();
        int r = wirehair_encode(wirehair_encoder, idx, buffers[idx], segment_size, &out_size);
        double t2 = get_cputime_ms();

        encode_time_total += t2 - t1;
        FATAL_ASSERT(r == 0);
        FATAL_ASSERT(segment_size == (int)out_size);
    }

    double after_encode_loop_time = get_cputime_ms();

    if (base_log) {
        fprintf(stderr, "<encode loop: %3f>", after_encode_loop_time - before_encode_loop_time);
    }

    encode_time_total = after_encode_loop_time - before_encode_loop_time;

    vector<int> shuffle;
    for (int i = 0; i < num_real + num_fec; i++) {
        shuffle.push_back(i);
    }

    for (int i = 0; i < (int)shuffle.size(); i++) {
        swap(shuffle[i], shuffle[my_rand() % shuffle.size()]);
    }

    string output(num_real * segment_size, 'b');
    WirehairCodec wirehair_decoder;
    double decode_time_total = 0;

    {
        double t1 = get_cputime_ms();
        wirehair_decoder = wirehair_decoder_create(0, num_real * segment_size, segment_size);
        double t2 = get_cputime_ms();
        decode_time_total += t2 - t1;
        if (base_log) {
            fprintf(stderr, "<decoder create: %.3f>", t2 - t1);
        }
    }

    int succ = 0;
    int decode_packet_cnt = 0;
    double before_decode_loop_time = get_cputime_ms();
    for (int i = num_fec + num_real - 1; i >= 0; i--) {
        int idx;
        if (g_use_shuffle) {
            idx = shuffle[i];
        } else {
            idx = i;
        }
        decode_packet_cnt++;
        int r = wirehair_decode(wirehair_decoder, idx, buffers[idx], segment_size);
        if (r == 0) {
            succ = 1;
            break;
        }
    }
    double after_decode_loop_time = get_cputime_ms();
    decode_time_total += after_decode_loop_time - before_decode_loop_time;
    if (base_log) {
        fprintf(stderr, "<decode loop: %.3f>", after_decode_loop_time - before_decode_loop_time);
    }

    FATAL_ASSERT(succ == 1);

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
    FATAL_ASSERT(input == output);

    free_buffers(buffers);
    wirehair_free(wirehair_encoder);
    wirehair_free(wirehair_decoder);

    if (base_log) {
        fprintf(stderr, "\n");
    }

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
    int round = 500;
    int segment_size = 1280;

    vector<int> num_fec_packets = {1, 2, 5, 10, 20, 50, 100, 200, 500};
    for (int i = 2; i < 256; i++) {
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

int wirehair_unittest(void) {
    wirehair_init();

    int segment_size = 1280;
    int round = 1000;

    for (int i = 0; i < round; i++) {
        int num_real = my_rand() % 1024 + 2;
        int num_fec = my_rand() % 1024;
        one_test(segment_size, num_real, num_fec);
    }
    return 0;
}

int wirehair_manualtest(void) {
    wirehair_init();

    get_timestamp_sec();

    performance_test();

    overhead_test();

    return 0;
}
