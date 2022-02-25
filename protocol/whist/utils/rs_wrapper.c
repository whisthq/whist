//#include <pthread.h>

#include <string.h>
#include <SDL2/SDL_thread.h>

#include "whist/core/whist.h"
#include "rs.h"
#include "rs_wrapper.h"

/*
============================
Defines
============================
*/

// the size of field our rs lib working on
#define RS_FIELD_SIZE 256

// size of row and column of RSTable
#define RS_TABLE_SIZE (RS_FIELD_SIZE)

// This is the type for the rs_table we use for caching
typedef RSCode *RSTable[RS_TABLE_SIZE][RS_TABLE_SIZE];

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

struct RSWrapper {
    int num_groups;                  // num of groups
    int num_real_buffers;            // num of total real buffers
    int num_fec_buffers;             // num of total FEC buffers
    int num_pending_groups;          // how many group havn't "received" enough buffers for decoding
    int group_max_num_real_buffers;  // the max num of real_buffers of all groups
    GroupInfo *group_infos;          // an array of GroupInfo
};

/*
============================
Globals
============================
*/

// Holds the RSTable for each thread
static SDL_TLSID rs_table_tls_id;

// the max size of group for encoding/decoding in rs_wrapper
// if num of buffers is large then this, rs_wrapper will split the encoding/decoding into multiple
// groups
// it's stored in a int instead of a macro, for easy testing
static int rs_wrapper_max_group_size = 128;

static int verbose_log = 0;

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          Gets an rs_code
 *
 * @param k                        The number of original packets
 * @param n                        The total number of packets
 *
 * @returns                        The RSCode for that (n, k) tuple
 */
static RSCode *get_rs_code(int k, int n);

/**
 * @brief                          Frees an RSTable
 *
 * @param opaque                   The RSTable* to free
 */
static void free_rs_code_table(void *opaque);

/*
below are two function are a very light innder wrapper of the rs lib, handle special case with dup
and dedup with the same interface
*/

// do rs_encode, or simply duplicate the buffer if k is 1
// when k is 1, n can be arbitrary large, otherwise k<=n<=256
static inline void rs_encode_or_dup(int k, int n, void *src[], void *dst, int index, int sz);

// do rs_decode, of simply deduo the buffer if k is 1
// when k is 1, n can be arbitrary large, otherwise k<=n<=256
static inline int rs_decode_or_dedup(int k, int n, void *pkt[], int index[], int sz);

// the inner version of rs_wrapper_create, let you control num of groups by yourself
static RSWrapper *rs_wrapper_create_inner(int num_real_buffers, int num_total_buffers,
                                          int num_groups);

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

/*
============================
Public Function Implementations
============================
*/

// init the rs wrapper system
void init_rs_wrapper(void) {
    static int initialized = 0;
    if (initialized == 0) {
        rs_table_tls_id = SDL_TLSCreate();
        init_rs();
        initialized = 1;
    }
}

// create a rs_wrapper
RSWrapper *rs_wrapper_create(int num_real_buffers, int num_total_buffers) {
    FATAL_ASSERT(num_real_buffers > 0);
    FATAL_ASSERT(num_real_buffers <= num_real_buffers);

    int num_fec_buffers = num_total_buffers - num_real_buffers;
    int num_groups =
        int_div_roundup(num_total_buffers, rs_wrapper_max_group_size);  // inter div, round up

    if (num_groups > num_real_buffers) {
        // if the num of groups needed is larger than MAX_RS_GROUP_SIZE
        // we can fail back to duplicate sending, and bypass the MAX_RS_GROUP_SIZE limitation
        num_groups = num_real_buffers;
    }

    return rs_wrapper_create_inner(num_real_buffers, num_total_buffers, num_groups);
}

// do encode with the rs_wrapper
void rs_wrapper_encode(RSWrapper *rs_wrapper, void **src, void **dst, int sz) {
    FATAL_ASSERT(rs_wrapper->num_groups > 0);

    // if num of groups is one, we can simply pass the encode request to the underlying lib without
    // maintain a lot of mappings
    if (rs_wrapper->num_groups == 1) {
        int k = rs_wrapper->group_infos[0].num_real_buffers;
        int n = k + rs_wrapper->group_infos[0].num_fec_buffers;

        for (int i = 0; i < rs_wrapper->num_fec_buffers; i++) {
            rs_encode_or_dup(k, n, src, dst[i], rs_wrapper->num_real_buffers + i, sz);
        }
        return;
    }

    // a buffer to store a subset of src
    void **src_sub = malloc(sizeof(void *) * rs_wrapper->group_max_num_real_buffers);

    // perform the encoding group-wise
    for (int i = 0; i < rs_wrapper->num_groups; i++) {
        // find the subset of src and store them into src_sub
        for (int j = 0; j < rs_wrapper->group_infos[i].num_real_buffers; j++) {
            int full_index = index_sub_to_full(rs_wrapper, i, j);
            src_sub[j] = src[full_index];
        }

        // give a short name for those two values
        int k = rs_wrapper->group_infos[i].num_real_buffers;
        int n = k + rs_wrapper->group_infos[i].num_fec_buffers;

        for (int j = 0; j < rs_wrapper->group_infos[i].num_fec_buffers; j++) {
            // map the indexes
            int sub_index = rs_wrapper->group_infos[i].num_real_buffers + j;
            int full_index = index_sub_to_full(rs_wrapper, i, sub_index);

            // do encoding with the underlying lib
            rs_encode_or_dup(k, n, src_sub, dst[full_index - rs_wrapper->num_real_buffers],
                             sub_index, sz);
        }
    }
    free(src_sub);
}

// do decode with the rs_wrapper
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

    // we run through the index arrary, to do the map mentioned above
    for (int i = 0; i < num_pkt; i++) {
        SubIndexInfo info = index_full_to_sub(rs_wrapper, index[i]);

        // already have enough for this subgroup, then "drop" extra buffers
        if (subgroup_offsets_cnt[info.group_id] ==
            rs_wrapper->group_infos[info.group_id].num_real_buffers)
            continue;

        subgroup_offsets[info.group_id][subgroup_offsets_cnt[info.group_id]] = i;
        subgroup_offsets_cnt[info.group_id]++;
    }

    // make a copy of the pkt array, since the underlying rs lib will reorder the input
    void **pkt_copy = malloc(sizeof(void *) * num_pkt);
    memcpy(pkt_copy, pkt, sizeof(void *) * num_pkt);

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

    // set extra pkt to NULL to avoid misuse
    for (int i = rs_wrapper->num_real_buffers; i < num_pkt; i++) {
        pkt[i] = NULL;
    }

    free(subgroup_offsets);
    free(pkt_copy);
    free(subgroup_offsets_cnt);
    free(pkt_sub);
    free(index_sub);

    return 0;
}

// destory the rs_wrapper
void rs_wrapper_destory(RSWrapper *rs_wrapper) {
    free(rs_wrapper->group_infos);
    free(rs_wrapper);
}

// init or reinit the counters inside rs_wrapper, so that you can use it from scratch
void rs_wrapper_decode_helper_reinit(RSWrapper *rs_wrapper) {
    rs_wrapper->num_pending_groups = rs_wrapper->num_groups;
    for (int i = 0; i < rs_wrapper->num_groups; i++) {
        rs_wrapper->group_infos[i].num_buffers_registered = 0;
    }
}

// register an index as "received"
void rs_wrapper_decode_helper_register_index(RSWrapper *rs_wrapper, int index) {
    SubIndexInfo info = index_full_to_sub(rs_wrapper, index);
    rs_wrapper->group_infos[info.group_id].num_buffers_registered++;
    if (rs_wrapper->group_infos[info.group_id].num_buffers_registered ==
        rs_wrapper->group_infos[info.group_id].num_real_buffers) {
        rs_wrapper->num_pending_groups--;
        FATAL_ASSERT(rs_wrapper->num_pending_groups >= 0);
    }
}

// detect if decode can happen
bool rs_wrapper_decode_helper_can_decode(RSWrapper *rs_wrapper) {
    FATAL_ASSERT(rs_wrapper->num_pending_groups >= 0);
    return rs_wrapper->num_pending_groups == 0;
}

// a function for test during developing
void rs_wrapper_test(void) {
    int num_real = 99, num_fec = 30;
    RSWrapper *rs_wrapper = rs_wrapper_create(num_real, num_real + num_fec);

    fprintf(stderr, "num_groups=%d\n", rs_wrapper->num_groups);
    for (int i = 0; i < num_real + num_fec; i++) {
        SubIndexInfo info = index_full_to_sub(rs_wrapper, i);
        int check = index_sub_to_full(rs_wrapper, info.group_id, info.sub_index);
    }
}

int rs_wrapper_set_max_group_size(int a) {
    int save = rs_wrapper_max_group_size;
    rs_wrapper_max_group_size = a;
    return save;
}

void rs_wrapper_set_verbose_log(int value) { verbose_log = value; }
/*
void rs_table_warmup(void)
{
    for(int i=1;i<RS_FIELD_SIZE;i++)
        for(int j=i;j<RS_FIELD_SIZE;j++)
        {
            get_rs_code(i,j);
        }
}*/

/*
============================
Private Function Implementations
============================
*/

static RSCode *get_rs_code(int k, int n) {
    FATAL_ASSERT(k <= n);
    FATAL_ASSERT(n <= RS_FIELD_SIZE);

    // Get the rs code table for this thread
    RSTable *rs_code_table = SDL_TLSGet(rs_table_tls_id);

    // If the table for this thread doesn't exist, initialize it
    if (rs_code_table == NULL) {
        rs_code_table = (RSTable *)safe_malloc(sizeof(RSTable));
        memset(rs_code_table, 0, sizeof(RSTable));
        SDL_TLSSet(rs_table_tls_id, rs_code_table, free_rs_code_table);
    }

    // If (n, k)'s rs_code hasn't been create yet, create it
    if ((*rs_code_table)[k][n] == NULL) {
        (*rs_code_table)[k][n] = rs_new(k, n);
    }

    // Now return the rs_code for (n, k)
    return (RSCode *)((*rs_code_table)[k][n]);
    // We make a redundant (RSCode*) because cppcheck parses the type wrong
}

static void free_rs_code_table(void *raw_rs_code_table) {
    RSTable *rs_code_table = (RSTable *)raw_rs_code_table;

    // If the table was never created, we have nothing to free
    if (rs_code_table == NULL) return;

    // Find any rs_code entries, and free them
    for (int i = 0; i < RS_TABLE_SIZE; i++) {
        for (int j = i; j < RS_TABLE_SIZE; j++) {
            if ((*rs_code_table)[i][j] != NULL) {
                rs_free((*rs_code_table)[i][j]);
            }
        }
    }

    // Now free the entire table
    free(rs_code_table);
}

inline static void rs_encode_or_dup(int k, int n, void *src[], void *dst, int index, int sz) {
    FATAL_ASSERT(k >= 0 && k < RS_FIELD_SIZE);
    FATAL_ASSERT(n >= 0);
    FATAL_ASSERT(index >= k && index < n);

    if (k == 1) {
        memcpy(dst, src[0], sz);
        return;
    }

    FATAL_ASSERT(n <= RS_FIELD_SIZE);
    RSCode *rs_code = get_rs_code(k, n);
    rs_encode(rs_code, src, dst, index, sz);
}
inline static int rs_decode_or_dedup(int k, int n, void *pkt[], int index[], int sz) {
    FATAL_ASSERT(k >= 0 && k < RS_FIELD_SIZE);
    FATAL_ASSERT(n >= 0);

    if (k == 1) {
        index[0] = 0;
        return 0;
    }

    FATAL_ASSERT(n <= RS_FIELD_SIZE);
    RSCode *rs_code = get_rs_code(k, n);
    return rs_decode(rs_code, pkt, index, sz);
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

    rs_wrapper->num_groups = num_groups;
    rs_wrapper->num_real_buffers = num_real_buffers;
    rs_wrapper->num_fec_buffers = num_fec_buffers;
    rs_wrapper->group_infos = malloc(sizeof(GroupInfo) * num_groups);

    fill_partition_plan(rs_wrapper);

    if (verbose_log) {
        fprintf(stderr, ">num_real_buffers=%d, num_fec_buffers=%d, num_groups=%d\n",
                num_real_buffers, num_fec_buffers, num_groups);
    }

    for (int i = 0; i < num_groups; i++) {
        FATAL_ASSERT(rs_wrapper->group_infos[i].num_real_buffers >= 0);
        rs_wrapper->group_max_num_real_buffers = max(rs_wrapper->group_max_num_real_buffers,
                                                     rs_wrapper->group_infos[i].num_real_buffers);

        if (rs_wrapper->group_max_num_real_buffers != 1) {
            // warm up the rs_table, so that some computation happens at creation time, so that
            // decode time can be faster.
            get_rs_code(rs_wrapper->group_max_num_real_buffers,
                        rs_wrapper->group_max_num_real_buffers +
                            rs_wrapper->group_infos[i].num_fec_buffers);
        }

        if (verbose_log) {
            // too spammy
            // fprintf(stderr,">>group_id=%d num_real_buffers=%d num_fec_buffers=%d\n",
            // i,rs_wrapper->group_infos[i].num_real_buffers ,
            // rs_wrapper->group_infos[i].num_fec_buffers );
        }
    }

    rs_wrapper_decode_helper_reinit(rs_wrapper);
    return rs_wrapper;
}
