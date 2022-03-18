#include <map>
#include <set>
#include <string>
using namespace std;

extern "C"
{
#include "audio.h"
#include "audio_path.h"
#include <whist/utils/threads.h>
#include <whist/utils/clock.h>
};

#include "whist/utils/atomic.h"
#include "string.h"
#include "assert.h"

static const int verbose_log=1;
static AudioContext *g_audio_context=0;
static WhistMutex g_mutex;
static atomic_int cached_device_queue_len;
static string last_packet_data;

//how many frames allowed to queue inside the audio device/
const int max_num_inside_device_queue=10;

const int max_num_inside_user_queue=10;

//how many frames allowed to queue intotal 
//const int max_num_total=max_num_inside_device_queue + max_num_inside_user_queue; 

//a dedicated thread for audio render
static int multi_threaded_audio_renderer(void *)
{
    while(1)
    {
        if(render_audio(g_audio_context)!=0)
        {
            whist_sleep(1);
        }   
    }
    return 0;

}

int audio_path_init(void)
{
    g_mutex = whist_create_mutex();
    g_audio_context = init_audio();

    atomic_init(&cached_device_queue_len,max_num_inside_device_queue);

    whist_create_thread(multi_threaded_audio_renderer, "MultiThreadedAudioRenderer", NULL);

    return 0;
}


//the core data struct of the audio queue is an ordered map (rb tree)
//it's used as a queue to allow insert at any position, but pop must be in order
static map<int,string> mp;

//a larger buffer used for anit-replay purpose
static set<int> anti_replay;


int detect_skip_num(int user_queue_len, int device_queue_len)
{
    int num_device_queue_empty_slots= max_num_inside_device_queue- device_queue_len;

    if(num_device_queue_empty_slots<0)
    {
        num_device_queue_empty_slots=0;
    }

    //when we hit the hard limit we remove 
    if(user_queue_len>= max_num_inside_user_queue + num_device_queue_empty_slots)
    {
        return max_num_inside_user_queue;
    }

    return 0;
}

// how many packets needs to be actively flushed
int active_flush_cnt=0;
bool doing_active_flush=0;

// how many packets are skiped by catch_up_skip
int skiped_cnt=0;

//last id pushed to decoder, used for track packet loss
int last_decoded_id=-1;

int push_to_audio_path(int id, unsigned char *buf, int size)
{

    int device_queue_len=atomic_load(&cached_device_queue_len);
    string s(buf,buf+size);

    whist_lock_mutex(g_mutex);

    const int max_reorder_allowed=5;
    //TODO better handling of reorder
    //when serious reoroder is detected, we can dely sending data to the device
    //so that the audio queue can put packets into correct order

    if(id+max_reorder_allowed<last_decoded_id||anti_replay.find(id)!=anti_replay.end()) //already have this id
    {
        whist_unlock_mutex(g_mutex);
        return -1;
    }

    {
        const int anti_replay_size=2000;
        anti_replay.insert(id);
        while(anti_replay.size()>anti_replay_size) anti_replay.erase(anti_replay.begin());
    }

    mp[id]=s;
    
    int user_queue_len=(int)mp.size();
    int expected_skip=detect_skip_num(user_queue_len, device_queue_len);

    if(expected_skip >0)
    {

        if(verbose_log)
            fprintf(stderr,"queue size=%d, has to skip!!\n",(int)mp.size());

        for(int i=0;i<expected_skip;i++)
        {
            if((int)mp.size() <=active_flush_cnt)
            {
                    if(verbose_log)
                    {
                        fprintf(stderr,"this usually shouldn't happen %d %d %d %d\n",(int)mp.size(),active_flush_cnt, i,expected_skip);
                    }
                    break;
            }
            //assert((int)mp.size() >active_flush_cnt );
            assert(!mp.empty());
            mp.erase(mp.begin());

        }
    }

    whist_unlock_mutex(g_mutex);

    return 0;
}


void pop_inner(unsigned char *buf, int *size)
{
    assert(!mp.empty());
    auto it=mp.begin();

    
    if(last_decoded_id +1 !=it->first)
    {
        if(verbose_log) fprintf(stderr, "lost (or reordered) packet %d!!!\n", last_decoded_id+1);
    }

    last_decoded_id=it->first;
    memcpy(buf,it->second.c_str(),it->second.length());
    *size=(int)it->second.length();
    last_packet_data=it->second;
    mp.erase(it);
}

//when the device is running really low, dup last frame to make it higher
//the finaly version might be random with a probility depending on the queue length
//at the moment we use a simpler strategy
bool decide_dup_frame(int device_queue_len)
{
    //disabled
    //return 0;

    if(device_queue_len  > max_num_inside_device_queue/2 ) return 0;

    //else free_cap is larger than half

    static timestamp_us last_fill_time=0;
    timestamp_us now= current_time_us ();

    //only dup every 1000ms
    if(now-last_fill_time>1000*1000) 
    {   
        last_fill_time=now;
        
        return 1;
    }
    return 0;

}

//when the audio queue is running high, drop some packets from the queue
//the finaly version might be random with a probility depending on the queue length
//at the moment we use a simpler strategy
bool decide_early_drop(int queue_len)
{
    int free_cap= max_num_inside_user_queue -queue_len;
    if(free_cap<0) free_cap=0;

    if(free_cap > max_num_inside_user_queue/2)
    {
        return 0;
    }

    static timestamp_us last_drop_time=0;
    timestamp_us now= current_time_us ();

    //only dup every 1000ms
    if(now-last_drop_time>1000*1000) 
    {   
        last_drop_time=now;
        
        return 1;
    }
    return 0;

}

int pop_from_audio_path( unsigned char *buf, int *size)
{
    int ret=-1;
    
    int device_queue_byte=safe_get_audio_queue(g_audio_context);
    int device_queue_len=(device_queue_byte + DECODED_BYTES_PER_FRAME-1)/DECODED_BYTES_PER_FRAME;
    atomic_store(&cached_device_queue_len,device_queue_len );

    if(device_queue_byte==0&&verbose_log) 
    {
        static timestamp_us last_log_time=0;
        timestamp_us now= current_time_us ();
        if(now-last_log_time>200*1000) 
        {   
            last_log_time=now;
            fprintf(stderr,"buffer becomes empty!!!!\n");
        }
    }

    if(verbose_log) 
    {
        static timestamp_us last_log_time=0;
        timestamp_us now= current_time_us ();
        if(now-last_log_time>100*1000) 
        {
            last_log_time=now;
            fprintf(stderr,"%d %d %d\n",(int)mp.size(),device_queue_len,device_queue_byte);
        }
    }

    whist_lock_mutex(g_mutex);
    if(device_queue_byte<0)
    {
        mp.clear(); //if audio device is not ready drop all packets
        active_flush_cnt=0;
        doing_active_flush=0;
        whist_unlock_mutex(g_mutex);
        return -1;
    }

    //unconditionally flush packets
    if(doing_active_flush)
    {
        assert((int)mp.size()>=active_flush_cnt);
        pop_inner(buf,size);
        active_flush_cnt--;
        if(active_flush_cnt==0)
            doing_active_flush=false;
        whist_unlock_mutex(g_mutex);
        return 0;
    }

    //when ever device buffer reaches 0, we start to queue packet for anti-jitter
    //and flush the queued packet activately later
    if(device_queue_byte==0) 
    {
        if(mp.size()< max_num_inside_device_queue) 
        {
            active_flush_cnt=  (int)mp.size() ;
            whist_unlock_mutex(g_mutex); // wait for more packets
            return -2;
        }
        else  //we have queued enough
        {
            pop_inner(buf,size);
            
            active_flush_cnt=max_num_inside_device_queue-1;
            doing_active_flush=true;
            skiped_cnt=0;//reset this, since we are starting another round

            whist_unlock_mutex(g_mutex);
            return 0;
        }
    }
    else 
    {
        //there is no packet in audio queue
        if(mp.size()==0)
        {
            
            if(decide_dup_frame(device_queue_len)&& last_packet_data.length()) 
            {
                fprintf(stderr,"device_queue running low, len=%d, filled with last frame!!\n",device_queue_len);
                memcpy(buf,last_packet_data.c_str(),last_packet_data.length());
                *size=(int)last_packet_data.length();
                whist_unlock_mutex(g_mutex);
                return 0;
            }

            whist_unlock_mutex(g_mutex);
            return -4;
        }

        //if the device's buffer is avaliable for another packet
        //we feed imediately
        if(device_queue_len<max_num_inside_device_queue) 
        {
            pop_inner(buf,size);
            whist_unlock_mutex(g_mutex);
            return 0;
        }
        //then we have device_queue is full
        else if(decide_early_drop(mp.size()))
        {
            int fake_size=0;
            fprintf(stderr,"aduio_queue running high, len=%d, dropped one frame!!\n",(int)mp.size());
            pop_inner(buf,&fake_size); //drop this packet
            whist_unlock_mutex(g_mutex);
            return -5;
        }
        else
        {
            whist_unlock_mutex(g_mutex);
            return -3;
        }
    }
    assert(0==1);//should not reach

    return ret;
}

