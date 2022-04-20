#include <string.h>
#include <SDL2/SDL_thread.h>

#include <whist/core/whist.h>
#include "cm256/gf256_cpuinfo.h"
#include "cm256/cm256.h"
#include "rs_common.h"
#include "lugi_rs_extra.h"
#include "rs_wrapper.h"

/*
============================
Defines
============================
*/

// a list of RS Implementations
typedef enum { LUGI_RS = 0, CM256 = 1, NUM_RS_IMPLEMENTATIONS = 2 } RSImplementation;

// the info of position of where a buffer locates in the groups
typedef struct {
    int group_id;   // id of group
    int sub_index;  // index of buffer inside group
} SubIndexInfo;

// stores the info of a group
typedef struct {
    int num_real_buffers;        // num of real buffers of the group
    int num_fec_buffers;         // num of fec buffers of the group
    int num_buffers_registered;  // stores how many buffers we have "received" for this group, to
                                 // help detect if decoding can happen, in O(1) time
} GroupInfo;

// the main struct of RSWrapper
struct RSWrapper {
    int num_groups;                  // num of groups
    int num_real_buffers;            // num of total real buffers
    int num_fec_buffers;             // num of total FEC buffers
    int num_pending_groups;          // how many group havn't "received" enough buffers for decoding
    int group_max_num_real_buffers;  // the max num of real_buffers of all groups
    int group_max_num_fec_buffers;   // the max num of fec_buffers of all groups
    GroupInfo *group_infos;          // an array of GroupInfo
};

/*
============================
Globals
============================
*/

// the implementation to use
static RSImplementation rs_implementation_to_use = CM256;

// the max size of group for encoding/decoding in rs_wrapper
// if num of total buffers is large than this, rs_wrapper will split the encoding/decoding into
// multiple groups, so that each group does not exceed the below size; it's stored in an int instead
// of a macro, for easy testing; client and server should have same value
static int rs_wrapper_max_group_size = 256;

// another limitation for group spliting, after split the "overhead" of each group will not exceed
// this value. set overhead_of_group() function for the definition of overhead
// with 20.0, each group's size will roughly at original + fec of 100+25 / 40+40.
static double rs_wrapper_max_group_overhead = 20.0;

// only for debug and testing
static int verbose_log = 0;

/*
============================
Private Function Declarations
============================
*/

/*
two function below are interfaces to talk with the RS libs. In addition they handle special case
with dup and dedup
*/

/**
 * @brief                          do RS encode with the RS wrapper, handle k=1 with dup
 *
 * @param k                        num of original buffers
 * @param n                        num of total buffers (original+redundant)
 * @param src                      an arrary of original buffers
 * @param dst                      an arrary of redundant buffers, the memeroy should be already
 *                                 allocated before passing here
 * @param sz                       size of buffers
 *
 * @note                           when k is 1, n can be arbitrary large, otherwise k<=n<=256
 */
static void rs_encode_or_dup(int k, int n, void *src[], void *dst[], int sz);

/**
 * @brief                          do RS decode with the RS wrapper
 *
 * @param k                        num of original buffers
 * @param n                        num of total buffers (original+redundant)
 * @param rs_wrapper               RSwrapper object created by rs_wrapper_create()
 * @param pkt                      an array of input buffers, may contain orginal buffer and
 *                                 redundant buffer in any order. used as output as well
 * @param index                    an array of index of buffers
 * @param sz                       size of buffers
 *
 * @returns                        zero on success
 *
 * @note                           after success, all orginal buffers are store inside the
 *                                 first k position of pkt, in correct
 *                                 order. when k is 1, n can be arbitrary large, otherwise
 *                                 k<=n<=256
 */
static int rs_decode_or_dedup(int k, int n, void *pkt[], int index[], int sz);

/*
the below 3 functions are highly associated, they decide how to partition the buffers, and how to do
the mapping
*/

/**
 * @brief                          do RS decode with the RS wrapper
 *
 * @param rs_wrapper               (input and output) RSwrapper object created by
 *                                 rs_wrapper_create()
 * @note                           the function reads basics parameters inside rs_wrapper, and fill
 *                                 the partition info into rs_wrapper
 */
static void fill_partition_plan(RSWrapper *rs_wrapper);

/**
 * @brief                          map the full index into sub index (and group_id)
 *
 * @param rs_wrapper               RSwrapper object created by rs_wrapper_create()
 * @param index                    the full index before splitting into groups
 *
 * @returns                        group id, with sub_index inside group
 */
static SubIndexInfo index_full_to_sub(RSWrapper *rs_wrapper, int index);

/**
 * @brief                          map the group_id and sub_index into full index
 *
 * @param rs_wrapper               RSwrapper object created by rs_wrapper_create()
 * @param group_id                 the id of group, i.e. the index of group
 * @param sub_index                the sub index inside group
 *
 * @returns                        the full index without groups involved
 */
static int index_sub_to_full(RSWrapper *rs_wrapper, int group_id, int sub_index);

/**
 * @brief                          the inner version of rs_wrapper_create, with on extra parameter,
 *                                 allow you to control of num of groups
 *
 * @param num_real_buffers         num of original buffers
 * @param num_total_buffers        num of buffers in total
 * @param num_groups               num of groups spliting into
 *
 * @returns                        The RS wrapper
 */
static RSWrapper *rs_wrapper_create_inner(int num_real_buffers, int num_total_buffers,
                                          int num_groups);

/**
 * @brief                          defines and calculates the "overhead" of a group
 *
 * @param num_real_buffers         num of original buffers of group
 * @param num_fec_buffer           num of fec buffers of group
 *
 * @returns                        The "overhead" of group
 *
 * @note                           the "overhead" here is defined as roughly how much "unit" of
 *                                 computation is spent on average on each input+output bytes.
 *                                 see details in the implemnetation of this function below
 */
static double overhead_of_group(int num_real_buffers, int num_fec_buffers);

/**
 * @brief                          a helper function to help decide partiton of an integer.
 *
 * @param integer_to_partition     the integer to partition
 * @param group_id                 the id of group
 * @param group_num                the num of groups
 * @returns                        The num of buffers inside the group with the above id
 *
 * @note                           fill_partition_plan() calls this function inside.
 *                                 fill_partition_plan aims at partition plan of FEC
 *                                 this function is only a helper function aims at partition
 *                                 an integer
 */
static int int_partition_helper(int integer_to_partition, int group_id, int group_num);

/*
============================
Public Function Implementations
============================
*/

int init_rs_wrapper(void) {
    static int initialized = 0;
    if (initialized == 1) return 0;

    switch (rs_implementation_to_use) {
        case CM256: {
            CpuInfo cpu_info = gf256_get_cpuinfo();
            LOG_INFO("gf256 detected CPU type is %s", cpu_type_to_str(cpu_info.cpu_type));
            LOG_INFO("cpu_info: has_avx2=%d has_ssse3=%d has_sse2=%d has_neon=%d",
                     cpu_info.has_avx2, cpu_info.has_ssse3, cpu_info.has_sse2, cpu_info.has_neon);

            if (cpu_info.cpu_type == CPU_TYPE_X86 || cpu_info.cpu_type == CPU_TYPE_X64) {
                // cpu without avx2 is not rare.
                // if cpu doesn't support avx2, with ssse3 it's only 30% slow.
                // so this is only a warning, and the lib will fallback to ssse3
                if (!cpu_info.has_avx2) {
                    LOG_WARNING("CPU type is x86/x64 but AVX2 is not supported!");
                }
                // we assume cpu without ssse3 is rare.
                // And simply yeild a fatal here
                // If this causes a problem for developing, consider make it a WARNING,
                // then the lib will fallback to sse2, but it's much slower.
                if (!cpu_info.has_ssse3) {
                    LOG_FATAL("CPU type is x86/x64 but SSSE3 is not supported!");
                }
            } else if (cpu_info.cpu_type == CPU_TYPE_ARM32 || cpu_info.cpu_type == CPU_TYPE_ARM64) {
                // the only arm platform we support is M1 and above
                // they should all support neon.
                // the lib has the ability of fallback to pure c for arm, but arm without
                // neon is unexpected, so still yeild a fatal here.
                if (!cpu_info.has_neon) {
                    LOG_FATAL("CPU type is arm32/arm64 but neon is not supported!");
                }
            } else {
                LOG_FATAL("unknown CPU type");
            }

            FATAL_ASSERT(cm256_init() == 0);
            break;
        }
        case LUGI_RS: {
            lugi_rs_extra_init();
            break;
        }
        default: {
            LOG_FATAL("unknown RS implentation value %d\n", (int)rs_implementation_to_use);
        }
    }

    initialized = 1;
    return 0;
}

RSWrapper *rs_wrapper_create(int num_real_buffers, int num_total_buffers) {
    FATAL_ASSERT(num_real_buffers > 0);
    FATAL_ASSERT(num_real_buffers <= num_real_buffers);

    int num_fec_buffers = num_total_buffers - num_real_buffers;

    int num_groups = -1;

    // try each num of groups start from 1 to num_real_buffers - 1
    // see if the num satifise the max_group_size and max_group_overhead limitation
    for (int i = 1; i < num_real_buffers; i++) {
        // compute the max num of packets inside each group, the assumption is the groups are
        // partitoned evenly
        int max_num_real_in_groups = int_div_roundup(num_real_buffers, i);
        int max_num_fec_in_groups = int_div_roundup(num_fec_buffers, i);

        // if this num of groups satisfies both the max_group_size and max_group_overhead
        // limitation, use this num
        if ((max_num_real_in_groups + max_num_fec_in_groups <= rs_wrapper_max_group_size) &&
            (overhead_of_group(max_num_real_in_groups, max_num_fec_in_groups) <=
             rs_wrapper_max_group_overhead)) {
            num_groups = i;
            break;
        }
    }

    // if all attempt above fails, we always have the choice of doing duplicate sending by sending
    // num_groups same as num_real_buffers
    if (num_groups == -1) {
        num_groups = num_real_buffers;
    }

    // call inner function to do the rest works
    return rs_wrapper_create_inner(num_real_buffers, num_total_buffers, num_groups);
}

void rs_wrapper_encode(RSWrapper *rs_wrapper, void **src, void **dst, int sz) {
    FATAL_ASSERT(rs_wrapper->num_groups > 0);

    // if num of groups is one, we can simply pass the encode request to the underlying lib without
    // maintain a lot of mappings
    if (rs_wrapper->num_groups == 1) {
        int k = rs_wrapper->group_infos[0].num_real_buffers;
        int n = k + rs_wrapper->group_infos[0].num_fec_buffers;

        // call encode interface
        rs_encode_or_dup(k, n, src, dst, sz);
        return;
    }

    // buffers to store a subset of src and dst
    void **src_sub = safe_malloc(sizeof(void *) * rs_wrapper->group_max_num_real_buffers);
    void **dst_sub = safe_malloc(sizeof(void *) * rs_wrapper->group_max_num_fec_buffers);

    // perform the encoding group-wise
    for (int i = 0; i < rs_wrapper->num_groups; i++) {
        // find the subset of src for current group and store them into src_sub
        for (int j = 0; j < rs_wrapper->group_infos[i].num_real_buffers; j++) {
            int full_index = index_sub_to_full(rs_wrapper, i, j);
            src_sub[j] = src[full_index];
        }
        // find the subset of dst for current group and store them into dst_sub
        for (int j = 0; j < rs_wrapper->group_infos[i].num_fec_buffers; j++) {
            int sub_index = rs_wrapper->group_infos[i].num_real_buffers + j;
            int full_index = index_sub_to_full(rs_wrapper, i, sub_index);
            dst_sub[j] = dst[full_index - rs_wrapper->num_real_buffers];
        }

        // give a short name for those two values
        int k = rs_wrapper->group_infos[i].num_real_buffers;
        int n = k + rs_wrapper->group_infos[i].num_fec_buffers;

        // call encode interface
        rs_encode_or_dup(k, n, src_sub, dst_sub, sz);
    }

    free(src_sub);
    free(dst_sub);
}

int rs_wrapper_decode(RSWrapper *rs_wrapper, void **pkt, int *index, int num_pkt, int sz) {
    FATAL_ASSERT(rs_wrapper->num_groups > 0);

    // if num of groups is one, we can simply pass the decode request to the underlying lib without
    // maintain a lot of mappings
    if (rs_wrapper->num_groups == 1) {
        int k = rs_wrapper->group_infos[0].num_real_buffers;
        int n = k + rs_wrapper->group_infos[0].num_fec_buffers;
        int res = rs_decode_or_dedup(k, n, pkt, index, sz);
        FATAL_ASSERT(res == 0);
        return 0;
    }

    // helper buffers to map offsets into corresponding groups
    // "offsets" means the position in index
    // possible optimizations: move all malloc into rs_wrapper create.
    // those arrays only contain meta info, won't be a bottleneck,
    // so for simplicity, we just allocation in decode call.
    int **subgroup_offsets = safe_malloc(sizeof(int *) * rs_wrapper->num_groups);
    int *subgroup_offsets_cnt = safe_malloc(sizeof(int) * rs_wrapper->num_groups);

    // more allocation and init of the help buffers
    for (int i = 0; i < rs_wrapper->num_groups; i++) {
        subgroup_offsets[i] =
            safe_malloc(sizeof(int) * rs_wrapper->group_infos[i].num_real_buffers);
        subgroup_offsets_cnt[i] = 0;
    }

    // make a copy of the pkt array, since the underlying rs lib will reorder the input
    void **pkt_copy = safe_malloc(sizeof(void *) * num_pkt);
    memcpy(pkt_copy, pkt, sizeof(void *) * num_pkt);

    int unused_cnt = 0;
    // we run through the index arrary, to do the map mentioned above
    for (int i = 0; i < num_pkt; i++) {
        SubIndexInfo info = index_full_to_sub(rs_wrapper, index[i]);

        // already have enough for this subgroup, then ignore extra buffers.
        // and simply put them to the end of pkt array
        if (subgroup_offsets_cnt[info.group_id] ==
            rs_wrapper->group_infos[info.group_id].num_real_buffers) {
            pkt[rs_wrapper->num_real_buffers + unused_cnt] = pkt_copy[i];
            unused_cnt++;
            continue;
        }

        subgroup_offsets[info.group_id][subgroup_offsets_cnt[info.group_id]] = i;
        subgroup_offsets_cnt[info.group_id]++;
    }

    // two helper buffers to store subsets of pkt and index
    void **pkt_sub = safe_malloc(sizeof(void *) * rs_wrapper->group_max_num_real_buffers);
    int *index_sub = safe_malloc(sizeof(int) * rs_wrapper->group_max_num_real_buffers);

    // perform the decoding group-wise
    for (int i = 0; i < rs_wrapper->num_groups; i++) {
        FATAL_ASSERT(subgroup_offsets_cnt[i] == rs_wrapper->group_infos[i].num_real_buffers);

        // find the subset of pkt store into pkt_sub
        // find the subset of index, and store the mapped one into index_sub
        for (int j = 0; j < rs_wrapper->group_infos[i].num_real_buffers; j++) {
            int offset = subgroup_offsets[i][j];
            pkt_sub[j] = pkt_copy[offset];
            SubIndexInfo info = index_full_to_sub(rs_wrapper, index[offset]);
            FATAL_ASSERT(info.group_id == i);
            index_sub[j] = info.sub_index;
        }

        // give a short name for those two values
        int k = rs_wrapper->group_infos[i].num_real_buffers;
        int n = k + rs_wrapper->group_infos[i].num_fec_buffers;

        int res = rs_decode_or_dedup(k, n, pkt_sub, index_sub, sz);
        FATAL_ASSERT(res == 0);

        // save pkt_sub to final place of pkt
        for (int j = 0; j < rs_wrapper->group_infos[i].num_real_buffers; j++) {
            pkt[index_sub_to_full(rs_wrapper, i, j)] = pkt_sub[j];
        }
        free(subgroup_offsets[i]);  // we dont need the buffer for this group any more
    }

    free(subgroup_offsets);
    free(pkt_copy);
    free(subgroup_offsets_cnt);
    free(pkt_sub);
    free(index_sub);

    return 0;
}

void rs_wrapper_destroy(RSWrapper *rs_wrapper) {
    free(rs_wrapper->group_infos);
    free(rs_wrapper);
}

void rs_wrapper_decode_helper_register_index(RSWrapper *rs_wrapper, int index) {
    // calculate the corresponding group
    SubIndexInfo info = index_full_to_sub(rs_wrapper, index);

    // increase the counter of regisitered buffer inside the corrresponding group
    rs_wrapper->group_infos[info.group_id].num_buffers_registered++;

    // if that group get enough packets for decoding
    if (rs_wrapper->group_infos[info.group_id].num_buffers_registered ==
        rs_wrapper->group_infos[info.group_id].num_real_buffers) {
        // decreases the counter of groups pending
        rs_wrapper->num_pending_groups--;
        FATAL_ASSERT(rs_wrapper->num_pending_groups >= 0);
    }
}

bool rs_wrapper_decode_helper_can_decode(RSWrapper *rs_wrapper) {
    FATAL_ASSERT(rs_wrapper->num_pending_groups >= 0);
    // if num of pending groups becomes 0, then we can decoder
    return rs_wrapper->num_pending_groups == 0;
}

void rs_wrapper_decode_helper_reset(RSWrapper *rs_wrapper) {
    // reset all the counters
    rs_wrapper->num_pending_groups = rs_wrapper->num_groups;
    for (int i = 0; i < rs_wrapper->num_groups; i++) {
        rs_wrapper->group_infos[i].num_buffers_registered = 0;
    }
}

int rs_wrapper_set_max_group_size(int a) {
    int save = rs_wrapper_max_group_size;
    if (a > 0) {
        rs_wrapper_max_group_size = a;
    }
    return save;
}

double rs_wrapper_set_max_group_overhead(double a) {
    int save = rs_wrapper_max_group_overhead;
    if (a > 0) {
        rs_wrapper_max_group_overhead = a;
    }
    return save;
}

void rs_wrapper_set_verbose_log(int value) { verbose_log = value; }

/*
============================
Private Function Implementations
============================
*/

static void rs_encode_or_dup(int k, int n, void *src[], void *dst[], int sz) {
    FATAL_ASSERT(k >= 0 && k < RS_FIELD_SIZE);
    FATAL_ASSERT(n >= 0);
    FATAL_ASSERT(k <= n);

    // the num of fec packets
    int fec_num = n - k;

    // handle k==1 case with duplication
    if (k == 1) {
        for (int i = 0; i < fec_num; i++) {
            memcpy(dst[i], src[0], sz);
        }
        return;
    }

    FATAL_ASSERT(n <= RS_FIELD_SIZE);

    // call the underlying lib according to rs_implementation_to_use
    switch (rs_implementation_to_use) {
        case CM256: {
            cm256_encoder_params params;
            params.OriginalCount = k;
            params.RecoveryCount = n - k;
            params.BlockBytes = sz;

            cm256_block *blocks = safe_malloc(params.OriginalCount * sizeof(cm256_block));

            // TODO inefficient code
            for (int i = 0; i < params.OriginalCount; i++) {
                blocks[i].Block = src[i];
                blocks[i].Index = i;
            }

            for (int i = 0; i < fec_num; i++) {
                cm256_encode_block(params, blocks, k + i, dst[i]);
            }
            free(blocks);
            break;
        }
        case LUGI_RS: {
            RSCode *rs_code = lugi_rs_extra_get_rs_code(k, n);
            for (int i = 0; i < fec_num; i++) {
                rs_encode(rs_code, src, dst[i], k + i, sz);
            }
            break;
        }
        default: {
            LOG_FATAL("unknown RS implentation value %d\n", (int)rs_implementation_to_use);
        }
    }
}

static int rs_decode_or_dedup(int k, int n, void *pkt[], int index[], int sz) {
    FATAL_ASSERT(k >= 0 && k < RS_FIELD_SIZE);
    FATAL_ASSERT(n >= 0);
    FATAL_ASSERT(k <= n);

    // handle k==1 case with dedup
    if (k == 1) {
        index[0] = 0;
        return 0;
    }
    FATAL_ASSERT(n <= RS_FIELD_SIZE);

    // call the underlying lib according to rs_implementation_to_use
    switch (rs_implementation_to_use) {
        case CM256: {
            cm256_encoder_params params;
            params.OriginalCount = k;
            params.RecoveryCount = n - k;
            params.BlockBytes = sz;

            cm256_block *blocks = safe_malloc(params.OriginalCount * sizeof(cm256_block));

            for (int i = 0; i < params.OriginalCount; i++) {
                blocks[i].Index = index[i];
                blocks[i].Block = pkt[i];
            }
            int r = cm256_decode(params, blocks);

            FATAL_ASSERT(r == 0);

            for (int i = 0; i < params.OriginalCount; i++) {
                pkt[blocks[i].Index] = (char *)blocks[i].Block;
            }

            free(blocks);
            return r;
        }
        case LUGI_RS: {
            RSCode *rs_code = lugi_rs_extra_get_rs_code(k, n);
            return rs_decode(rs_code, pkt, index, sz);
        }
        default: {
            LOG_FATAL("unknown RS implentation value %d\n", (int)rs_implementation_to_use);
        }
    }
}

static int int_partition_helper(int total, int group_id, int group_num) {
    if (group_id >= total) {
        return 0;
    }
    return int_div_roundup(total - group_id, group_num);
}

static void fill_partition_plan(RSWrapper *rs_wrapper) {
    int num_groups = rs_wrapper->num_groups;
    int num_real_buffers = rs_wrapper->num_real_buffers;
    int num_fec_buffers = rs_wrapper->num_fec_buffers;

    // get num of original buffers and fec buffers for each group
    for (int i = 0; i < num_groups; i++) {
        rs_wrapper->group_infos[i].num_real_buffers =
            int_partition_helper(num_real_buffers, i, num_groups);
        rs_wrapper->group_infos[i].num_fec_buffers =
            int_partition_helper(num_fec_buffers, i, num_groups);
    }
}

static SubIndexInfo index_full_to_sub(RSWrapper *rs_wrapper, int index) {
    FATAL_ASSERT(0 <= index && index < rs_wrapper->num_real_buffers + rs_wrapper->num_fec_buffers);

    // index mapping to group id and sub_index
    SubIndexInfo info;
    if (index < rs_wrapper->num_real_buffers) {
        info.group_id = index % rs_wrapper->num_groups;
        info.sub_index = index / rs_wrapper->num_groups;
    } else {
        info.group_id = (index - rs_wrapper->num_real_buffers) % rs_wrapper->num_groups;
        info.sub_index = rs_wrapper->group_infos[info.group_id].num_real_buffers +
                         (index - rs_wrapper->num_real_buffers) / rs_wrapper->num_groups;
    }
    return info;
}

static int index_sub_to_full(RSWrapper *rs_wrapper, int group_id, int sub_index) {
    FATAL_ASSERT(0 <= group_id && group_id < rs_wrapper->num_groups);

    // group id and sub_index mapping back
    if (sub_index < rs_wrapper->group_infos[group_id].num_real_buffers) {
        return group_id + sub_index * rs_wrapper->num_groups;
    } else {
        return rs_wrapper->num_real_buffers + group_id +
               (sub_index - rs_wrapper->group_infos[group_id].num_real_buffers) *
                   rs_wrapper->num_groups;
    }
}

static RSWrapper *rs_wrapper_create_inner(int num_real_buffers, int num_total_buffers,
                                          int num_groups) {
    FATAL_ASSERT(num_real_buffers > 0);
    FATAL_ASSERT(num_real_buffers <= num_real_buffers);
    FATAL_ASSERT(num_groups <= num_real_buffers);

    RSWrapper *rs_wrapper = (RSWrapper *)safe_zalloc(sizeof(RSWrapper));

    int num_fec_buffers = num_total_buffers - num_real_buffers;

    // init elementary data
    rs_wrapper->num_groups = num_groups;
    rs_wrapper->num_real_buffers = num_real_buffers;
    rs_wrapper->num_fec_buffers = num_fec_buffers;
    rs_wrapper->group_infos = safe_malloc(sizeof(GroupInfo) * num_groups);

    // get the partition plan, based on the elementary data
    fill_partition_plan(rs_wrapper);

    if (verbose_log) {
        fprintf(stderr, ">num_real_buffers=%d, num_fec_buffers=%d, num_groups=%d\n",
                num_real_buffers, num_fec_buffers, num_groups);
    }

    // check correctness of partition, calculate group_max_num_real_buffers and
    // group_max_num_fec_buffers
    for (int i = 0; i < num_groups; i++) {
        FATAL_ASSERT(rs_wrapper->group_infos[i].num_real_buffers >= 0);
        rs_wrapper->group_max_num_real_buffers = max(rs_wrapper->group_max_num_real_buffers,
                                                     rs_wrapper->group_infos[i].num_real_buffers);
        rs_wrapper->group_max_num_fec_buffers =
            max(rs_wrapper->group_max_num_fec_buffers, rs_wrapper->group_infos[i].num_fec_buffers);

        // print out partition info, commented since too spammy
        /*
        if (verbose_log) {

            fprintf(stderr,">>group_id=%d num_real_buffers=%d num_fec_buffers=%d\n",
            i,rs_wrapper->group_infos[i].num_real_buffers ,
            rs_wrapper->group_infos[i].num_fec_buffers );
        }
        */
    }

    // set the decode helper counters to zero
    rs_wrapper_decode_helper_reset(rs_wrapper);
    return rs_wrapper;
}

static double overhead_of_group(int num_real_buffers, int num_fec_buffers) {
    // the "overhead" here is defined as roughly how much "unit" of computation is spent on average
    // on each input+output bytes

    // the (worst-case) decode time complexity is almost always >= encode, so we only optimize for
    // decode overhead

    double k = num_real_buffers;
    double m = num_fec_buffers;
    double l = MAX_PACKET_SEGMENT_SIZE;
    switch (rs_implementation_to_use) {
        case CM256: {
            // O(k^2) to invert a Cauchy Matrix
            // O(k*m*l) to generate recovery symbols
            // (k+m)*l bytes in total for input +output
            return (k * k + k * m * l) / ((k + m) * l);
        }
        case LUGI_RS: {
            // O(k^3) to invert a Vandermonde Matrix
            // O(k*m*l) to generate recovery symbols
            // (k+m)*l bytes in total for input +output
            return (k * k * k + k * m * l) / ((k + m) * l);
        }
        default: {
            LOG_FATAL("unknown RS implentation value %d\n", (int)rs_implementation_to_use);
        }
    }
}
