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
//#include <unistd.h>
using namespace std;

const int g_use_sleep = 0;
const int g_use_shuffle = 1;
const int g_segment_size = 4;

extern "C" {
#include "wirehair_test.h"
};

const int base_perf_log = 0;
const int verbose_log = 0;
const int run_on_ci = 1;

inline double get_timestamp_ms0(void) { return get_timestamp_sec() * 1000; }

static double get_timestamp_ms(void) {
#if OS_IS(OS_MACOS)
    struct timespec t2;
    // use CLOCK_MONOTONIC for relative time
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);

    double elapsed_time = (t2.tv_sec) * MS_IN_SECOND;
    elapsed_time += (t2.tv_nsec) / (double)NS_IN_MS;

    double ret = elapsed_time;
    return ret;
#else
    return 0;
#endif
}

int my_rand() {
    static random_device rd;
    static mt19937 g(rd());
    static uniform_int_distribution<> dis(1, 1000 * 1000 * 1000);
    return dis(g);
}

struct Encoder {
    int num_real_;
    int num_fec_;
    WirehairCodec code;
    void init(int num_real, int num_fec, int max_sz) {
        this->num_fec_ = num_fec;
        this->num_real_ = num_real;
    }
    void encode(char *src[], char *dst[], int sz) {
        vector<char> buf0(num_real_ * sz);

        char *buf = buf0.data();

        {
            double t1 = get_timestamp_ms();
            for (int i = 0; i < num_real_; i++) {
                memcpy(&buf[i * sz], src[i], sz);
            }
            double t2 = get_timestamp_ms();
            if (base_perf_log) {
                fprintf(stderr, "<encoder memcpy cost %f>", t2 - t1);
            }
        }

        {
            double t1 = get_timestamp_ms();
            code = wirehair_encoder_create(0, buf, num_real_ * sz, sz);
            double t2 = get_timestamp_ms();
            if (base_perf_log) {
                fprintf(stderr, "<encoder create cost %f>", (t2 - t1));
            }
        }

        unsigned int out_sz;
        for (int i = 0; i < num_fec_; i++) {
            double t1 = get_timestamp_ms();
            int r = wirehair_encode(code, num_real_ + i, dst[i], sz, &out_sz);
            double t2 = get_timestamp_ms();
            FATAL_ASSERT((int)out_sz == sz);
            if (verbose_log) {
                fprintf(stderr, "<encode idx=%d cost %f>", num_real_ + i, t2 - t1);
            }
        }

        if (verbose_log) fprintf(stderr, "\n");
    }
    void destory() { wirehair_free(code); }
};

struct Decoder {
    int num_real_;
    int num_fec_;
    WirehairCodec code;

    int ok = 0;

    int cnt = 0;

    void init(int num_real, int num_fec, int max_sz) {
        this->num_fec_ = num_fec;
        this->num_real_ = num_real;
        ok = 0;
        code = wirehair_decoder_create(0, num_real * max_sz, max_sz);
    }

    void feed(int index, char *pkt, int sz) {
        cnt++;
        double t1 = get_timestamp_ms();
        int r = wirehair_decode(code, index, pkt, sz);
        double t2 = get_timestamp_ms();
        if (r == 0) ok = 1;
        if (verbose_log) fprintf(stderr, "<decode feed cnt=%d cost %f>", cnt, t2 - t1);
    }

    int try_decode(char *pkt[], int sz) {
        if (ok == 0) return -1;

        vector<char> buf0(num_real_ * sz);
        // char *buf= &buf0[0];;
        char *buf = buf0.data();

        double t1 = get_timestamp_ms();
        wirehair_recover(code, buf, num_real_ * sz);
        double t2 = get_timestamp_ms();

        if (base_perf_log) {
            fprintf(stderr, "<decoder recover cost= %f>", t2 - t1);
        }
        for (int i = 0; i < num_real_; i++) {
            memcpy(pkt[i], &buf[i * sz], sz);
        }
        double t3 = get_timestamp_ms();
        if (base_perf_log) fprintf(stderr, "<decoder memcpy cost= %f>", t3 - t2);
        return 0;
    }
    void destory() { wirehair_free(code); }
};

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

    Encoder encoder;

    encoder.init(num_real, num_fec, segment_size);  // there is nothing inside to perf

    double encode_total;

    {
        double t1 = get_timestamp_ms();
        encoder.encode(&buffers[0], &buffers[num_real], segment_size);
        double t2 = get_timestamp_ms();
        if (base_perf_log) fprintf(stderr, "encode_time=%f; \n", (t2 - t1));
        encode_total = t2 - t1;
    }

    auto pkt = make_buffers(num_real, segment_size);

    Decoder decoder;

    double decode_total = 0;
    ;

    {
        double t1 = get_timestamp_ms();
        decoder.init(num_real, num_fec, segment_size);
        double t2 = get_timestamp_ms();
        if (base_perf_log) fprintf(stderr, "<decoder init cost= %f>", t2 - t1);
        decode_total += t2 - t1;
    }

    vector<int> shuffle;
    for (int i = 0; i < num_real + num_fec; i++) {
        shuffle.push_back(i);
    }

    for (int i = 0; i < (int)shuffle.size(); i++) {
        swap(shuffle[i], shuffle[my_rand() % shuffle.size()]);
    }

    int cnt = 0;
    {
        double t1 = get_timestamp_ms();
        int succ = 0;
        for (int i = num_fec + num_real - 1; i >= 0; i--) {
            // int idx=shuffle[i];
            int idx;
            if (g_use_shuffle)
                idx = shuffle[i];
            else
                idx = i;

            cnt++;

            decoder.feed(idx, buffers[idx], segment_size);

            int ret = decoder.try_decode(&pkt[0], segment_size) == 0;
            double tt3 = get_timestamp_ms();

            // fprintf(stderr,"<decode cnt=%d total cost %f>", cnt,tt3-tt1);
            if (ret) {
                if (verbose_log) fprintf(stderr, "\n");
                if (base_perf_log) fprintf(stderr, "decoded with %d packets; ", cnt);
                succ = 1;
                break;
            }
            // pkt.push_back(buffers[i]);
            // index.push_back(i);
        }
        assert(succ == 1);
        double t2 = get_timestamp_ms();
        decode_total += t2 - t1;
        if (base_perf_log) fprintf(stderr, "decode_time=%f\n", (t2 - t1));
    }

    string output;
    output = combine_copy(pkt, segment_size);

    assert(input.size() == output.size());
    assert(input == output);

    free_buffers(buffers);
    free_buffers(pkt);
    encoder.destory();
    decoder.destory();
    return make_tuple(cnt - num_real, encode_total, decode_total);
}

void overhead_test(void) {
    int round = 10000;

    if (run_on_ci) {
        round = 10;
    }

    vector<int> num_fec_packets = {1, 2, 5, 10, 20, 50, 100, 200, 500};
    for (int i = 2; i < 256; i++) {
        for (int ii = 0; ii < 3; ii++) {
            fprintf(stderr, "real=%d; ", i);
            for (int j = 1; j < (int)num_fec_packets.size(); j++) {
                int o1_cnt = 0, o2_cnt = 0, o3_cnt = 0;
                int o5_cnt = 0, o10_cnt = 0;
                int o_sum = 0;
                int y = num_fec_packets[j];
                int max_overhead = 0;
                for (int k = 0; k < round; k++) {
                    auto [overhead, encode_time, decode_time] = one_test(4, i, y);
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
                fprintf(stderr, "<%d;%3d,%3d,%2d,%2d,%2d;%3d;%3d>  ", y, o1_cnt, o2_cnt, o3_cnt,
                        o5_cnt, o10_cnt, max_overhead, o_sum);
            }
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    }
}

void performance_test(void) {
    int round = 5000;
    if (run_on_ci) round = 10;
    vector<int> num_fec_packets = {1, 2, 5, 10, 20, 50, 100, 200, 500};
    for (int i = 2; i < 256; i++) {
        for (int ii = 0; ii < 3; ii++) {
            fprintf(stderr, "real=%d; ", i);
            for (int j = 1; j < (int)num_fec_packets.size(); j++) {
                int y = num_fec_packets[j];

                double encode_time_min = 9999;
                double encode_time_max = 0;
                double encode_time_sum = 0;

                double decode_time_min = 9999;
                double decode_time_max = 0;
                double decode_time_sum = 0;

                for (int k = 0; k < round; k++) {
                    auto [overhead, encode_time, decode_time] = one_test(4, i, y);

                    encode_time_min = min(encode_time_min, encode_time);
                    encode_time_max = max(encode_time_max, encode_time);
                    encode_time_sum += encode_time;

                    decode_time_min = min(decode_time_min, decode_time);
                    decode_time_max = max(decode_time_max, decode_time);
                    decode_time_sum += decode_time;
                }

                fprintf(stderr, "<%3d; %.0f;%.0f,%.0f; %.0f,%.0f,%.0f>   ", y,
                        encode_time_min * 1000, encode_time_max * 1000,
                        encode_time_sum / round * 1000, decode_time_min * 1000,
                        decode_time_max * 1000, decode_time_sum / round * 1000);
            }
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    }
}

int wirehair_test(void) {
    wirehair_init();

    get_timestamp_sec();

    performance_test();

    overhead_test();

    return 0;
}
