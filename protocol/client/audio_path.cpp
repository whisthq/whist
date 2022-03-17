
#include <map>
#include <set>
#include <string>
using namespace std;

extern "C"
{
#include "audio_path.h"
#include <whist/utils/threads.h>
#include <whist/utils/clock.h>
#include "audio.h"
};
#include "string.h"
#include "assert.h"



static const int verbose_log=1;
static WhistMutex g_mutex;
static AudioContext *g_audio_context=0;

/*copied from audio.c*/
#define SAMPLES_PER_FRAME 480
#define BYTES_PER_SAMPLE 4
#define NUM_CHANNELS 2
#define DECODED_BYTES_PER_FRAME (SAMPLES_PER_FRAME * BYTES_PER_SAMPLE * NUM_CHANNELS)


// a sclient audio frame captured by some trick, we use it to fill lost packets 
unsigned char empty_frame[]={0x80,0xbb,0x00,0x00,0x92,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x8a,0x00,0x00,0x00,0xf4,0x5b,0x7d,0x4f,0x6b,0x90,0xe5,0xa1,0xd6,0x33,0x4d,0xd5,0x8f,0x8d,0xd5,0x1c,0x43,0x47,0x3f,0xb4,0x2b,0x49,0x96,0xd8,0xff,0xce,0x39,0xed,0x71,0xf6,0xc8,0x19,0xaa,0x0c,0x89,0xf2,0x13,0xb5,0xf5,0x3f,0x0f,0x46,0xe7,0x5b,0xed,0x92,0xfb,0xe7,0x29,0xaf,0xcc,0xeb,0x7f,0x02,0x15,0x2c,0xc8,0x3b,0x74,0x43,0x08,0xd9,0xd2,0xf9,0x3b,0x6a,0x81,0x06,0xb4,0xe9,0xf7,0x38,0xea,0xad,0x09,0x2a,0x9c,0x50,0xc6,0xda,0xfc,0x84,0xfa,0x35,0xc4,0x77,0xb3,0xba,0x24,0xd4,0xae,0x43,0xf9,0x04,0xd0,0xc0,0xff,0xa4,0xc3,0x0a,0x0a,0xea,0xce,0xa8,0x71,0x72,0x17,0x20,0x21,0xd9,0xc2,0x2c,0xfa,0x24,0x47,0x48,0x42,0xfc,0x4c,0x06,0x3e,0xae,0x39,0xea,0x27,0x75,0x0b,0x18,0x9e,0xff,0x49,0x8e,0x9f,0x05,0x02,0xfc,0xf3,0x30};

// the string to hold empty_frame[]
string fill_gap;

//use a dedicated thread for audio render
//this is not necessary, just experimental
int audio_queue_loop_thread(void *)
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

    int len=sizeof(empty_frame);
    fill_gap=string(empty_frame,empty_frame+len);
    //fill_gap=string(DECODED_BYTES_PER_FRAME,'\0');    

    whist_create_thread(audio_queue_loop_thread, "MultiThreadedDebugConsole", NULL);

    return 0;
}


//the core data struct of the audio queue is an ordered map (rb tree)
//it's used as a queue to allow insert at any position, but pop must be in order
static map<int,string> mp;

//a larger buffer used for anit-replay purpose
static set<int> anti_replay;

//how many frames allowed to queue at whist client
const int max_num_inside_audio_queue=10; 

//how many frames allowed to queue inside the audio device/
const int max_num_inside_device=10;

int detect_skip_num(int queue_len, int device_queue_len)
{

    int target= max_num_inside_audio_queue;

    if(device_queue_len< max_num_inside_device)
    {
        //when device queue len is low, it just means the packets has not been moved yet
        //add cap to max_num_inside_audio_queue
        target+=max_num_inside_device -device_queue_len;
    }

    //when we hit the hard limit we just drop half of all
    if(queue_len>=target)
    {
        return max_num_inside_audio_queue;
        //return max_num_inside_device/2;
    }

    return 0;

    /*
    //otherwise we do early random skip, if the num of free cap is smaller than half
    //the smaller num  of free caps, the large changes to drop packets
    
    int free_cap = max_num_inside_audio_queue -queue_len +1;
    if(free_cap <=0) free_cap=1;

    if(queue_len>max_num_inside_audio_queue*1/2  && rand()%( free_cap*free_cap  ) ==0)
        return 1;
    

    return 0;*/
}

//when max_num_inside_audio_queue is achived, how fast we skip
//small value leads to more pops, but each pop is shorter
//large value leads to less pops, but each pop is longer
//const int catch_up_skip=max_num_inside_audio_queue/2;

// how many packets needs to be actively flushed
int active_flush_cnt=0;
bool doing_active_flush=0;

// how many packets are skiped by catch_up_skip
int skiped_cnt=0;

//last id pushed to decoder, used for track packet loss
int last_decoded_id=-1;


int push_to_audio_path(int id, unsigned char *buf, int size)
{

    int device_queue_byte=safe_get_audio_queue(g_audio_context);
    int device_queue_len=(device_queue_byte + DECODED_BYTES_PER_FRAME-1)/DECODED_BYTES_PER_FRAME;

    string s(buf,buf+size);

    int ret=0;
    whist_lock_mutex(g_mutex);
    
    /*
    if(fill_gap.empty()) 
    {
        fill_gap=s;
        if(verbose_log)
        {
            for(int i=0;i<(int)s.length();i++)
            {
                fprintf(stderr,"0x%02x,",(int)(unsigned char)s[i]);
            }
            fprintf(stderr,"\n");
        }
    }*/

    const int max_reorder_allowed=3;
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

    //int capcity=(int)mp.size() -active_flush_cnt;
    //assert(capcity>=0);
    //when num of packets inside queue is larger than max_num_inside_audio_queue kick stale packets.
    //the num of active_flush_cnt doesn't count as capicity

    
    //new way
    int capcity=(int)mp.size();
    int expected_skip=detect_skip_num(capcity, device_queue_len );


    if(expected_skip >0)
    {
        //reset this or not???
        skiped_cnt=0;

        if(verbose_log)
            fprintf(stderr,"queue size=%d, has to skip!!\n",(int)mp.size());

        for(int i=0;i<expected_skip;i++)
        {
            assert((int)mp.size() >active_flush_cnt );

            assert(!mp.empty());

            mp.erase(mp.begin());
            skiped_cnt++;
            /*if(skipped_cnt>3)
                skiped_cnt=3;*/

        }
    }

    whist_unlock_mutex(g_mutex);

    return 0;
}




//enum State { WatingForMoreFrames, Normal };
//static State g_state=WatingForMoreFrames;

string last_packet_data;
//routine to pop a packet from queue
void pop_inner(unsigned char *buf, int *size)
{
    assert(!mp.empty());
    auto it=mp.begin();

    
    if(last_decoded_id +1 !=it->first && !doing_active_flush )
    {
        if(verbose_log) fprintf(stderr, "lost (or reordered) packet %d!!!\n", last_decoded_id+1);

        /*
        memcpy(buf,fill_gap.c_str(),fill_gap.length());
        *size=(int)fill_gap.length();
        last_decoded_id=last_decoded_id +1;

        return ;*/
        //the fill gap logic is disabled
        /*
        if(verbose_log) fprintf(stderr, "lost packet %d, skip_cnt=%d\n", last_decoded_id+1,skiped_cnt);
        
        if(true||skiped_cnt==0)
        {
                if(verbose_log)  fprintf(stderr, "filled with empty frames\n");
                memcpy(buf,fill_gap.c_str(),fill_gap.length());
                *size=(int)fill_gap.length();
                last_decoded_id=last_decoded_id +1;
                //whist_unlock_mutex(g_mutex);
                return ;
        }
        else if(skiped_cnt>0)
        {
            skiped_cnt--;
        }*/
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

    if(device_queue_len  > max_num_inside_device/2 ) return 0;

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

/*
    int less_than_half =  max_num_inside_device/2 - device_queue_len;
    
    if(less_than_half <=0 )  //then it means at least more than half
        return 0;
    
    return 1;
*/
}

//when the audio queue is running high, drop some packets from the queue
//the finaly version might be random with a probility depending on the queue length
//at the moment we use a simpler strategy
bool decide_early_drop(int queue_len)
{
    int free_cap= max_num_inside_audio_queue -queue_len;
    if(free_cap<0) free_cap=0;

    if(free_cap > max_num_inside_audio_queue/2)
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
    static int g_cnt=0;
    g_cnt++;

    int ret=-1;
    int queue_byte=safe_get_audio_queue(g_audio_context);
    int queue_len=(queue_byte + DECODED_BYTES_PER_FRAME-1)/DECODED_BYTES_PER_FRAME;

    if(queue_byte==0) if(verbose_log) 
    {
        static timestamp_us last_log_time=0;
        timestamp_us now= current_time_us ();
        if(now-last_log_time>200*1000) 
        {   last_log_time=now;
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
            fprintf(stderr,"%d %d %d\n",(int)mp.size(),queue_len,queue_byte);
        }
    }

    whist_lock_mutex(g_mutex);
    if(queue_byte<0)
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
    if(queue_byte==0) 
    {
        if(mp.size()< max_num_inside_device) 
        {
            active_flush_cnt=  (int)mp.size() ;
            whist_unlock_mutex(g_mutex); // wait for more packets
            return -2;
        }
        else  //we have queued enough
        {
            pop_inner(buf,size);
            
            active_flush_cnt=max_num_inside_device-1;
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
            
            if(decide_dup_frame(queue_len)&& last_packet_data.length()) 
            {
                fprintf(stderr,"device_queue running low, len=%d, filled with last frame!!\n",queue_len);
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
        if(queue_len<max_num_inside_device) 
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

