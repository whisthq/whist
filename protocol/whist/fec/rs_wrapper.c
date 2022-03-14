//#include <pthread.h>

#include <string.h>
#include <SDL2/SDL_thread.h>

#include "whist/core/whist.h"
#include "whist/fec/lugi_rs.h"
#include "whist/fec/cm256/cm256.h"
#include "rs_common.h"
#include "lugi_rs_helper.h"
#include "rs_wrapper.h"

/*
============================
Defines
============================
*/

// a list of RS Implementations
typedef enum { RS_LUGI = 0, CM256 = 1, NUM_RS_IMPLEMENTATIONS = 2 } RSImplementation;

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
                                 // help detect if decoding can happen in O(1) time
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
static RSImplementation rs_implementation = CM256;

// the max size of group for encoding/decoding in rs_wrapper
// if num of total buffers is large than this, rs_wrapper will split the encoding/decoding into
// multiple groups, so that each group does not exceed the below size; it's stored in an int instead
// of a macro, for easy testing; client and server should have same value
static int rs_wrapper_max_group_size = 256;

// another limitation for group spliting, after split the "overhead" of each group will not exceed
// this value. set overhead_of_group() function for the definition of overhead
static double rs_wrapper_max_group_overhead = 20;

static int verbose_log = 0;

/*
============================
Private Function Declarations
============================
*/

/*
two function below are interfaces to talk with the RS libs. In addition it handles special case with
dup and dedup
*/

// do rs_encode, or simply duplicate the buffer if k is 1
// when k is 1, n can be arbitrary large, otherwise k<=n<=256
static inline void rs_encode_or_dup(int k, int n, void *src[], void *dst[], int sz);

// do rs_decode, or simply deduo the buffer if k is 1
// when k is 1, n can be arbitrary large, otherwise k<=n<=256
static inline int rs_decode_or_dedup(int k, int n, void *pkt[], int index[], int sz);

/*
the below 3 functions are highly associated, they decide how to partition the buffers, and how to do
the mapping
*/

// generate a partition plan for the rs_wrapper, and fill that into rs_wrapper
static void fill_partition_plan(RSWrapper *rs_wrapper);

// map the full index into sub index (and group_id)
static SubIndexInfo index_full_to_sub(RSWrapper *rs_wrapper, int index);

// map the group_id and sub_index into full index
static int index_sub_to_full(RSWrapper *rs_wrapper, int group_id, int sub_index);

// the inner version of rs_wrapper_create, let you control num of groups by yourself
static RSWrapper *rs_wrapper_create_inner(int num_real_buffers, int num_total_buffers,
                                          int num_groups);

// defines the "overhead" of a group
static double overhead_of_group(int num_real_buffers, int num_fec_buffers);
/*
============================
Public Function Implementations
============================
*/

void init_rs_wrapper(void) {
    static int initialized = 0;
    if (initialized == 0) {
        lugi_rs_helper_init();
        FATAL_ASSERT(cm256_init() == 0);
        // TODO: produce a warning log if cm256 fails to use AVX2
        initialized = 1;
    }
}

RSWrapper *rs_wrapper_create(int num_real_buffers, int num_total_buffers) {
    FATAL_ASSERT(num_real_buffers > 0);
    FATAL_ASSERT(num_real_buffers <= num_real_buffers);

    int num_fec_buffers = num_total_buffers - num_real_buffers;

    int num_groups = -1;
    for (int i = 1; i < num_real_buffers; i++) {
        int max_num_real_in_groups = int_div_roundup(num_real_buffers, i);
        int max_num_fec_in_groups = int_div_roundup(num_fec_buffers, i);

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
    void **src_sub = malloc(sizeof(void *) * rs_wrapper->group_max_num_real_buffers);
    void **dst_sub = malloc(sizeof(void *) * rs_wrapper->group_max_num_fec_buffers);

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
    int **subgroup_offsets = malloc(sizeof(int *) * rs_wrapper->num_groups);
    int *subgroup_offsets_cnt = malloc(sizeof(int) * rs_wrapper->num_groups);

    // more allocation and init of the help buffers
    for (int i = 0; i < rs_wrapper->num_groups; i++) {
        subgroup_offsets[i] = malloc(sizeof(int) * rs_wrapper->group_infos[i].num_real_buffers);
        subgroup_offsets_cnt[i] = 0;
    }

    // make a copy of the pkt array, since the underlying rs lib will reorder the input
    void **pkt_copy = malloc(sizeof(void *) * num_pkt);
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
    void **pkt_sub = malloc(sizeof(void *) * rs_wrapper->group_max_num_real_buffers);
    int *index_sub = malloc(sizeof(int) * rs_wrapper->group_max_num_real_buffers);

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

void rs_wrapper_destory(RSWrapper *rs_wrapper) {
    free(rs_wrapper->group_infos);
    free(rs_wrapper);
}

void rs_wrapper_decode_helper_register_index(RSWrapper *rs_wrapper, int index) {
    SubIndexInfo info = index_full_to_sub(rs_wrapper, index);
    rs_wrapper->group_infos[info.group_id].num_buffers_registered++;
    if (rs_wrapper->group_infos[info.group_id].num_buffers_registered ==
        rs_wrapper->group_infos[info.group_id].num_real_buffers) {
        rs_wrapper->num_pending_groups--;
        FATAL_ASSERT(rs_wrapper->num_pending_groups >= 0);
    }
}

bool rs_wrapper_decode_helper_can_decode(RSWrapper *rs_wrapper) {
    FATAL_ASSERT(rs_wrapper->num_pending_groups >= 0);
    return rs_wrapper->num_pending_groups == 0;
}

void rs_wrapper_decode_helper_reset(RSWrapper *rs_wrapper) {
    rs_wrapper->num_pending_groups = rs_wrapper->num_groups;
    for (int i = 0; i < rs_wrapper->num_groups; i++) {
        rs_wrapper->group_infos[i].num_buffers_registered = 0;
    }
}

void rs_wrapper_test(void) {
    int num_real = 99, num_fec = 30;
    RSWrapper *rs_wrapper = rs_wrapper_create(num_real, num_real + num_fec);

    fprintf(stderr, "num_groups=%d\n", rs_wrapper->num_groups);
    for (int i = 0; i < num_real + num_fec; i++) {
        SubIndexInfo info = index_full_to_sub(rs_wrapper, i);
        int check = index_sub_to_full(rs_wrapper, info.group_id, info.sub_index);
        fprintf(stderr, "<index=%d check=%d>", i, check);
    }
    fprintf(stderr, "\n");
}

int rs_wrapper_set_max_group_size(int a) {
    int save = rs_wrapper_max_group_size;
    if (a > 0) {
        rs_wrapper_max_group_size = a;
    }
    return save;
}

double rs_wrapper_set_max_group_overhead(int a) {
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

inline static void rs_encode_or_dup(int k, int n, void *src[], void *dst[], int sz) {
    FATAL_ASSERT(k >= 0 && k < RS_FIELD_SIZE);
    FATAL_ASSERT(n >= 0);
    // FATAL_ASSERT(index >= k && index < n);
    FATAL_ASSERT(k <= n);

    int fec_num = n - k;

    if (k == 1) {
        for (int i = 0; i < fec_num; i++) {
            memcpy(dst[i], src[0], sz);
        }
        return;
    }

    FATAL_ASSERT(n <= RS_FIELD_SIZE);
    switch (rs_implementation) {
        case CM256: {
            cm256_encoder_params params;
            params.OriginalCount = k;
            params.RecoveryCount = n - k;
            params.BlockBytes = sz;

            cm256_block *blocks = malloc(params.OriginalCount * sizeof(cm256_block));

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
        case RS_LUGI: {
            RSCode *rs_code = get_rs_code(k, n);
            for (int i = 0; i < fec_num; i++) {
                rs_encode(rs_code, src, dst[i], k + i, sz);
            }
            break;
        }
        default: {
            LOG_FATAL("unknown RS implentation value %d\n", (int)rs_implementation);
        }
    }
}
inline static int rs_decode_or_dedup(int k, int n, void *pkt[], int index[], int sz) {
    FATAL_ASSERT(k >= 0 && k < RS_FIELD_SIZE);
    FATAL_ASSERT(n >= 0);
    FATAL_ASSERT(k <= n);

    if (k == 1) {
        index[0] = 0;
        return 0;
    }
    FATAL_ASSERT(n <= RS_FIELD_SIZE);

    switch (rs_implementation) {
        case CM256: {
            cm256_encoder_params params;
            params.OriginalCount = k;
            params.RecoveryCount = n - k;
            params.BlockBytes = sz;

            cm256_block *blocks = malloc(params.OriginalCount * sizeof(cm256_block));

            for (int i = 0; i < params.OriginalCount; i++) {
                blocks[i].Index = index[i];
                blocks[i].Block = pkt[i];
            }
            int r = cm256_decode(params, blocks);

            FATAL_ASSERT(r == 0);
            // if(r!=0) return r;

            for (int i = 0; i < params.OriginalCount; i++) {
                pkt[blocks[i].Index] = (char *)blocks[i].Block;
            }

            free(blocks);
            return r;
        }
        case RS_LUGI: {
            RSCode *rs_code = get_rs_code(k, n);
            return rs_decode(rs_code, pkt, index, sz);
        }
        default: {
            LOG_FATAL("unknown RS implentation value %d\n", (int)rs_implementation);
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

    for (int i = 0; i < num_groups; i++) {
        rs_wrapper->group_infos[i].num_real_buffers =
            int_partition_helper(num_real_buffers, i, num_groups);
        rs_wrapper->group_infos[i].num_fec_buffers =
            int_partition_helper(num_fec_buffers, i, num_groups);
    }
}

static SubIndexInfo index_full_to_sub(RSWrapper *rs_wrapper, int index) {
    FATAL_ASSERT(0 <= index && index < rs_wrapper->num_real_buffers + rs_wrapper->num_fec_buffers);

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
    if (sub_index < rs_wrapper->group_infos[group_id].num_real_buffers) {
        return group_id + sub_index * rs_wrapper->num_groups;
    } else {
        return rs_wrapper->num_real_buffers + group_id +
               (sub_index - rs_wrapper->group_infos[group_id].num_real_buffers) *
                   rs_wrapper->num_groups;
    }
}

// create a rs_wrapper, while let you control the num_groups by yourself
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
    rs_wrapper->group_infos = malloc(sizeof(GroupInfo) * num_groups);

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
    // most of RS lib has a encode time complexity and a worse decode time complexity of
    // O(num_real_buffers*num_fec_buffers). the "overhead" here is defined as how much "unit" of
    // computation is spent on average on each buffer (original + fec)
    return ((double)num_real_buffers * num_fec_buffers) / (num_real_buffers + num_fec_buffers);
}
