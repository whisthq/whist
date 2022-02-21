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

//TODO: remove in PR
//temp solution for the spamming when FATAL_ASSERT fails in test
#undef FATAL_ASSERT
#include <assert.h>
#define FATAL_ASSERT assert

#define RS_TABLE_SIZE 256  // size of row and column

#define MAX_RS_GROUP_SIZE 30
#define OPTIMIZATION 0

// This is the type for the rs_table we use for caching
typedef RSCode* RSTable[RS_TABLE_SIZE][RS_TABLE_SIZE];

typedef struct 
{
    int subgroup_id;
    int sub_index;
}IndexInfo;

typedef struct 
{
    int num_real_buffers;
    int num_fec_buffers;
    int num_buffers_registered;
}SubGroupInfo;

struct RSWrapper
{
    int num_sub_groups;
    int num_real_buffers;
    int num_fec_buffers;
    int num_pending_subgroups;
    int group_max_num_real_buffers;
    SubGroupInfo * sub_group_infos;
    RSCode ** rs_codes;
};


/*
============================
Globals
============================
*/

// Holds the RSTable for each thread
static SDL_TLSID rs_table_tls_id;

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
static RSCode* get_rs_code(int k, int n);

/**
 * @brief                          Frees an RSTable
 *
 * @param opaque                   The RSTable* to free
 */
static void free_rs_code_table(void* opaque);

static IndexInfo index_to_sub(RSWrapper *rs_wrapper,int index);
static int sub_to_index(RSWrapper *rs_wrapper,int subgroup_id, int sub_index);
static int int_distribute_helper(int total, int start_pos,int num);


/*
============================
Public Function Implementations
============================
*/

void init_rs_wrapper(void) {
    static int initialized=0;
    if(initialized==0)
    {
        rs_table_tls_id = SDL_TLSCreate();
        init_rs();
        initialized=1;
    }
}

RSWrapper * rs_wrapper_create(int num_real_buffers, int num_total_buffers)
{
    //fprintf(stderr, "<%d,%d>\n",num_real_buffers,  num_total_buffers );

    FATAL_ASSERT(num_real_buffers>0);
    FATAL_ASSERT(num_real_buffers<=num_real_buffers);
    RSWrapper * rs_wrapper=(RSWrapper *)safe_zalloc(sizeof(RSWrapper));
    //int num_total_packet=num_real_buffers + num_fec_buffers;
    int num_fec_buffers= num_total_buffers -num_real_buffers;
    int num_sub_groups=int_div_roundup(num_total_buffers,MAX_RS_GROUP_SIZE);  //inter div, round up
    rs_wrapper->num_sub_groups=  num_sub_groups;
    rs_wrapper->num_real_buffers=num_real_buffers;
    rs_wrapper->num_fec_buffers=num_fec_buffers;
    rs_wrapper->sub_group_infos=malloc(sizeof(SubGroupInfo)*num_sub_groups);
    rs_wrapper->rs_codes=malloc(sizeof(RSCode*)*num_sub_groups);
    
    for(int i=0;i<num_sub_groups;i++)
    {
        rs_wrapper->sub_group_infos[i].num_real_buffers= int_distribute_helper(num_real_buffers,i,num_sub_groups);
        rs_wrapper->sub_group_infos[i].num_fec_buffers = int_distribute_helper(num_fec_buffers,i,num_sub_groups);
        if(rs_wrapper->sub_group_infos[i].num_real_buffers)
        {
            rs_wrapper->group_max_num_real_buffers=max(rs_wrapper->group_max_num_real_buffers,rs_wrapper->sub_group_infos[i].num_real_buffers );

            rs_wrapper->rs_codes[i]=get_rs_code( rs_wrapper->sub_group_infos[i].num_real_buffers, rs_wrapper->sub_group_infos[i].num_real_buffers+rs_wrapper->sub_group_infos[i].num_fec_buffers);
        }
        else
        {
            // if we got a zero here, that means (num_real+num_fec)/MAX_RS_GROUP_SIZE > num_real,
            // which futher measns  num_fec/num_real > (MAX_RS_GROUP_SIZE-1),
            // since MAX_RS_GROUP_SIZE is assumed to be large,
            // that num_fec/num_real is a super huge redendant ratio, there must be something wrong with the ratio itself
            // we can hadnle this extreme case, but no need to waste time
            LOG_FATAL("num_real_buffers=%d num_fec_buffers=%d, the redundant ratio is too large to be supported.\n", num_real_buffers, num_fec_buffers );
        }
    }

    rs_wrapper_can_decode_reinit(rs_wrapper);
    return rs_wrapper;
}

void rs_wrapper_encode(RSWrapper * rs_wrapper, void **src, void**dst, int sz)
{
    FATAL_ASSERT(rs_wrapper->num_sub_groups>0);
#if OPTIMIZATION
    if(rs_wrapper->num_sub_groups==1)
    {
        for(int i=0;i<rs_wrapper->num_fec_buffers;i++)
        {
            //fprintf(stderr, "<%d>\n",rs_wrapper->num_real_buffers + i );
            rs_encode(rs_wrapper->rs_codes[0],src,dst[i], rs_wrapper->num_real_buffers + i,sz);
        }
        return;
    }
#endif
    void ** sub_src= malloc ( sizeof(void *) * rs_wrapper->group_max_num_real_buffers );
    for(int i=0;i<rs_wrapper->num_sub_groups;i++)
    {

        //void ** sub_dst =malloc( sizeof(void*) * rs_wrapper->sub_group_infos[i].num_fec_buffers );
        for(int j=0;j<rs_wrapper->sub_group_infos[i].num_real_buffers;j++)
        {
            int full_index=sub_to_index(rs_wrapper,i,j);
            sub_src[j]=src[full_index];
        }
        for(int j=0;j<rs_wrapper->sub_group_infos[i].num_fec_buffers;j++)
        {
            int sub_index=rs_wrapper->sub_group_infos[i].num_real_buffers+ j;
            int full_index=sub_to_index(rs_wrapper,i,sub_index);
            rs_encode(rs_wrapper->rs_codes[i],sub_src,dst[full_index -rs_wrapper->num_real_buffers], sub_index, sz);
        }
    }
}

int rs_wrapper_decode(RSWrapper * rs_wrapper, void **pkt, int * index, int num_pkt, int sz)
{
     FATAL_ASSERT(rs_wrapper->num_sub_groups>0);
     fprintf(stderr,"<<%d ,num_pkt=%d>>", rs_wrapper->num_sub_groups,num_pkt);
#if OPTIMIZATION
     if(rs_wrapper->num_sub_groups==1)
     {
         int res=rs_decode(rs_wrapper->rs_codes[0],pkt,index,sz);
         FATAL_ASSERT(res==0);
         return 0;
     }
#endif

     void **pkt_copy=  malloc( sizeof(void *) *num_pkt );
     memcpy(pkt_copy,pkt,sizeof(void *)*num_pkt);

     int **subgroup_offsets;
     int *subgroup_offsets_cnt;
     subgroup_offsets= malloc( sizeof(int *) * rs_wrapper->num_sub_groups);
     subgroup_offsets_cnt= malloc(sizeof(int) * rs_wrapper->num_sub_groups);

     for(int i=0;i<rs_wrapper->num_sub_groups;i++)
     {
        subgroup_offsets[i]=malloc(sizeof(int) *rs_wrapper->sub_group_infos[i].num_real_buffers );
        subgroup_offsets_cnt[i]=0;
     }


     for(int i=0;i<num_pkt;i++)
     {
         IndexInfo info= index_to_sub(rs_wrapper, index[i]);

         //already have enough for this subgroup
         if(subgroup_offsets_cnt[info.subgroup_id]== rs_wrapper->sub_group_infos[info.subgroup_id].num_real_buffers) continue; 

         subgroup_offsets[info.subgroup_id][ subgroup_offsets_cnt[info.subgroup_id] ]=i;
         subgroup_offsets_cnt[info.subgroup_id]++;
     }

     void **pkt_sub;
     int*index_sub;
     pkt_sub=malloc(sizeof(void*) *rs_wrapper->group_max_num_real_buffers);
     index_sub=malloc (sizeof(int) * rs_wrapper->group_max_num_real_buffers);
     fprintf(stderr,"<<max_real=%d>>\n",rs_wrapper->group_max_num_real_buffers);

     for(int i=0;i<rs_wrapper->num_sub_groups;i++)
     {
         fprintf(stderr,"current_sub_group=%d\n",i);
         FATAL_ASSERT( subgroup_offsets_cnt[i]== rs_wrapper->sub_group_infos[i].num_real_buffers );
         for(int j=0;j<rs_wrapper->sub_group_infos[i].num_real_buffers;j++)
         {
             int offset=subgroup_offsets[i][j];
             //fprintf(stderr,"offset=%d\n",offset);
             //fprintf(stderr,"%d j=%d max_num_real=%d\n",rs_wrapper->sub_group_infos[i].num_real_buffers,j,max_num_real_buffers);
             pkt_sub[j]=pkt_copy[  offset  ];
             //fprintf(stderr,"offset=%d again\n",offset);
             IndexInfo info= index_to_sub(rs_wrapper, index[offset] );
             FATAL_ASSERT(info.subgroup_id==i);
             index_sub[j]= info.sub_index;
                          
             //fprintf(stderr,"<offset=%d, index_sub=%d>\n",offset,index_sub[j]);
             //fprintf(stderr,"[[%d]]\n",max_num_real_buffers);
         }
         int res=rs_decode(rs_wrapper->rs_codes[i], pkt_sub,index_sub,sz);
         FATAL_ASSERT(res==0);
         for(int j=0;j<rs_wrapper->sub_group_infos[i].num_real_buffers;j++)
         {
             pkt[ sub_to_index(rs_wrapper, i,j) ]= pkt_sub[j];
         }
     }
     for(int i=rs_wrapper->num_real_buffers;i<num_pkt;i++)
     {
         pkt[i]=NULL;// set extra pkt to NULL to avoid misuse
     }

     return 0;
}
static void rs_wrapper_destory(RSWrapper * wrapper)
{
    //
}


void rs_wrapper_can_decode_reinit(RSWrapper * rs_wrapper)
{
    rs_wrapper->num_pending_subgroups=rs_wrapper->num_sub_groups;
    for(int i=0;i<rs_wrapper->num_sub_groups;i++)
    {
        rs_wrapper->sub_group_infos[i].num_buffers_registered=0;
    }
}
void rs_wrapper_can_decode_register_index(RSWrapper * rs_wrapper,int index)
{
    IndexInfo info=index_to_sub(rs_wrapper,index);
    rs_wrapper->sub_group_infos[info.subgroup_id].num_buffers_registered++;
    if(rs_wrapper->sub_group_infos[info.subgroup_id].num_buffers_registered== rs_wrapper->sub_group_infos[info.subgroup_id].num_real_buffers)
    {
        rs_wrapper->num_pending_subgroups--;
        FATAL_ASSERT(rs_wrapper->num_pending_subgroups>=0);
    }
}

bool rs_wrapper_can_decode(RSWrapper * rs_wrapper)
{
        FATAL_ASSERT(rs_wrapper->num_pending_subgroups>=0);
    return rs_wrapper->num_pending_subgroups==0;
}

void rs_wrapper_test(void)
{
    int num_real=99,num_fec=30;
    RSWrapper *rs_wrapper= rs_wrapper_create(num_real,num_real+num_fec);

    fprintf(stderr, "num_sub_groups=%d\n", rs_wrapper->num_sub_groups);
    for(int i=0; i<num_real+num_fec ;i++)
    {
        IndexInfo info = index_to_sub(rs_wrapper,i);
        int check= sub_to_index(rs_wrapper,info.subgroup_id,info.sub_index);

        //fprintf(stderr,"i=%d   subgroup_id=%d   sub_index=%d   check=%d\n",i,info.subgroup_id, info.sub_index, check);
    }
}

/*
============================
Private Function Implementations
============================
*/

static RSCode* get_rs_code(int k, int n) {
    FATAL_ASSERT(k <= n);

    // Get the rs code table for this thread
    RSTable* rs_code_table = SDL_TLSGet(rs_table_tls_id);

    // If the table for this thread doesn't exist, initialize it
    if (rs_code_table == NULL) {
        rs_code_table = (RSTable*)safe_malloc(sizeof(RSTable));
        memset(rs_code_table, 0, sizeof(RSTable));
        SDL_TLSSet(rs_table_tls_id, rs_code_table, free_rs_code_table);
    }

    // If (n, k)'s rs_code hasn't been create yet, create it
    if ((*rs_code_table)[k][n] == NULL) {
        (*rs_code_table)[k][n] = rs_new(k, n);
    }

    // Now return the rs_code for (n, k)
    return (RSCode*)((*rs_code_table)[k][n]);
    // We make a redundant (RSCode*) because cppcheck parses the type wrong
}

static void free_rs_code_table(void* raw_rs_code_table) {
    RSTable* rs_code_table = (RSTable*)raw_rs_code_table;

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


static IndexInfo index_to_sub(RSWrapper *rs_wrapper,int index)
{
    FATAL_ASSERT( 0<=index  && index< rs_wrapper->num_real_buffers +rs_wrapper->num_fec_buffers );

    IndexInfo info;
    if(index<rs_wrapper->num_real_buffers)
    {
        info.subgroup_id = index%rs_wrapper->num_sub_groups;
        info.sub_index= index/ rs_wrapper->num_sub_groups;
    }
    else
    {
        info.subgroup_id = (index-rs_wrapper->num_real_buffers)%rs_wrapper->num_sub_groups;
        info.sub_index=  rs_wrapper->sub_group_infos[info.subgroup_id].num_real_buffers+(index-rs_wrapper->num_real_buffers)/rs_wrapper->num_sub_groups;
    }
    return info;
}

static int sub_to_index(RSWrapper *rs_wrapper,int subgroup_id, int sub_index)
{
    FATAL_ASSERT(0<=subgroup_id && subgroup_id< rs_wrapper->num_sub_groups);
    if(sub_index<rs_wrapper->sub_group_infos[subgroup_id].num_real_buffers)
    {
        return subgroup_id + sub_index* rs_wrapper->num_sub_groups;
    }
    else
    {
        return rs_wrapper->num_real_buffers  +  subgroup_id + (sub_index- rs_wrapper->sub_group_infos[subgroup_id].num_real_buffers) * rs_wrapper->num_sub_groups;
    }
}

static int int_distribute_helper(int total, int start_pos,int num)
{
    if(start_pos >=total){
        return 0;
    }
    return int_div_roundup(total-start_pos, num);
}
