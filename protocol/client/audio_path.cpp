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
#include <whist/logging/logging.h>
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
const int max_num_inside_device_queue=9;

//how many frames allowed to  queue inside the user queue
const int max_num_inside_user_queue=9;

const int max_total_queued=max_num_inside_device_queue + max_num_inside_user_queue;
const int target_total_queued=max_num_inside_device_queue;

const double queue_len_management_sensitivity=1.5;

enum ManangeOperation
{
    EarlyDrop=0,
    EarlyDup=1,
    NoOp=2
};

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

    atomic_init(&cached_device_queue_len,0);
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
    if(device_queue_len<0)
    {
        //drop everything if audio device is not initilized
        return user_queue_len;
    }

    int total_len=user_queue_len+device_queue_len;

    if(total_len>= max_total_queued)
    {
        //when we hit the hard limit we remove everything in excess,
        //to let the queue len go back to normal
        int skip_num = total_len- target_total_queued;
        if( skip_num>user_queue_len )
        {
            skip_num=user_queue_len;
        }
        return skip_num;
    }

    return 0;
}

// how many packets needs to be actively flushed
int active_flush_cnt=0;
bool doing_active_flush=0;

// how many packets are skiped by catch_up_skip
int skiped_cnt=0;

//last id pushed to decoder, used for track packet loss
int last_popped_id=-1;

int push_to_audio_path(int id, unsigned char *buf, int size)
{

    int device_queue_len=atomic_load(&cached_device_queue_len);
    if(device_queue_len<0)
    {
        return -1;
    }

    string s(buf,buf+size);

    whist_lock_mutex(g_mutex);

    const int max_reorder_distant_allowed=4;
    //TODO better handling of reorder
    //when serious reoroder is detected, we can dely sending data to the device
    //so that the audio queue can put packets into correct order

    if(id+max_reorder_distant_allowed<last_popped_id||anti_replay.find(id)!=anti_replay.end()) //already have this id
    {
        whist_unlock_mutex(g_mutex);
        return -1;
    }

    {
        const int anti_replay_size=1000;
        anti_replay.insert(id);
        while(anti_replay.size()>anti_replay_size) anti_replay.erase(anti_replay.begin());
    }

    mp[id]=s;
    
    int user_queue_len=(int)mp.size();
    int total_queue_len=user_queue_len+device_queue_len;

    int expected_skip=detect_skip_num(user_queue_len, device_queue_len);

    if(expected_skip >0)
    {

        if(verbose_log)
        {
            fprintf(stderr,"queue size=%d %d %d, has to skip %d!!\n",total_queue_len,user_queue_len, device_queue_len, expected_skip);
        }

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

    
    if(last_popped_id +1 !=it->first)
    {
        if(verbose_log) fprintf(stderr, "lost (or reordered) packet %d!!!\n", last_popped_id+1);
    }

    last_popped_id=it->first;
    memcpy(buf,it->second.c_str(),it->second.length());
    *size=(int)it->second.length();
    last_packet_data=it->second;
    mp.erase(it);
}


ManangeOperation decide_queue_len_manage_operation(int user_queue_len,int device_queue_len)
{
    const int target_sample_times=10;
    const int sample_period=100*1000;
    //keep each value instead of running sum for easy debugging
    static int sampled_queue_lens[target_sample_times +1];
    static int current_sample_cnt=0;
    static timestamp_us last_sample_time=0;

    FATAL_ASSERT(device_queue_len>0);
    //when calling this function, device_queue_len should never be <=zero,
    //guarenteed by upper level logic

    if(active_flush_cnt>0)
    {
        current_sample_cnt=0;
        return NoOp;
    }

    int total_len=user_queue_len+device_queue_len;
    timestamp_us now=current_time_us();

    if(now >= last_sample_time + sample_period)
    {
        sampled_queue_lens[current_sample_cnt]= total_len;
        current_sample_cnt++;
        last_sample_time=now;
    }

    FATAL_ASSERT(current_sample_cnt<=target_sample_times);

    if(current_sample_cnt<target_sample_times)
    {
        return NoOp;
    }

    //otherwise we have  current_sample_cnt =target_sample_times;

    double sum=0;
    for(int i=0;i<target_sample_times;i++)
    {
        sum+= sampled_queue_lens[i];  
    }
    double avg_len=sum/target_sample_times;

    //no matter which case, begin next sample period
    current_sample_cnt=0;

    ManangeOperation op=NoOp;
    if(avg_len >= target_total_queued + queue_len_management_sensitivity)
    {
        if(verbose_log)
        {
            fprintf(stderr,"aduio_queue running high, len=%.2f %d %d %d, drop one frame!!; ",avg_len, total_len, user_queue_len, device_queue_len);
        }
        op= EarlyDrop;
    }
    else if(avg_len <= target_total_queued - queue_len_management_sensitivity)
    {
        if(verbose_log)
        {
            fprintf(stderr,"aduio_queue low high, len=%.2f %d %d %d, fill with last frame!! ",avg_len, total_len, user_queue_len, device_queue_len);
        }
        op= EarlyDup;
    }

    if(verbose_log && op!=NoOp)
    {
        fprintf(stderr,"last 10 sampled length=[");
        for(int i=0;i<target_sample_times;i++)
        {
            fprintf(stderr,"%d,",sampled_queue_lens[i]);
        }
        fprintf(stderr,"]\n");
    }

    return op;
}

int pop_from_audio_path( unsigned char *buf, int *size)
{
    int ret=-1;
    
    int device_queue_byte=get_device_audio_queue_bytes(g_audio_context);
    int device_queue_len=-1;
    if(device_queue_byte>=0)
    {
        device_queue_len=(device_queue_byte + DECODED_BYTES_PER_FRAME-1)/DECODED_BYTES_PER_FRAME;
    }

    atomic_store(&cached_device_queue_len,device_queue_len );

    if(verbose_log && device_queue_byte==0) 
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

    int user_queue_len=(int)mp.size();

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
        auto op= decide_queue_len_manage_operation(user_queue_len,device_queue_len);

        if(op==EarlyDup)
        {
            if(last_packet_data.length())
            {
                memcpy(buf,last_packet_data.c_str(),last_packet_data.length());
                *size=(int)last_packet_data.length();
                whist_unlock_mutex(g_mutex);
                return 0;
            }
            //otherwise don't return continue running as normal
        }
        else if(op==EarlyDrop)
        {
            if(user_queue_len>0)
            {
                int fake_size=0;
                pop_inner(buf,&fake_size); //drop this packet
            }
            //don't return after early drop, continue to run as normal
        }

        //there is no packet in audio queue
        if(mp.size()==0)
        {
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
        else
        {
            whist_unlock_mutex(g_mutex);
            return -3;
        }
        assert(0==1);//should not reach
    }
    assert(0==1);//should not reach

    return ret;
}
