#include <vector>
#include <random>

#include "wrapper.h"

#include <string.h>
#include <assert.h>
#include <unistd.h>
using namespace std;

const int g_use_sleep=0;
const int g_use_shuffle=0;
const int g_segment_size=1280;
const int g_num_real=512;
const int g_num_fec=512;
const int g_iterations=20;

const int g_num_mem_test_segments=500;  // only for mem test
				      // yancey@yanceys-MacBook-Air FEC-benchmark %

extern "C"
{
#include "wirehair_test.h"
};

typedef unsigned long long u64_t;

u64_t get_current_time_ms()
{
	timespec tmp_time;
	clock_gettime(CLOCK_MONOTONIC, &tmp_time);
	return tmp_time.tv_sec*1000ll+tmp_time.tv_nsec/(double)1000l/(double)1000;
}


static double get_cpu_time_ms(void) {
    struct timespec t2;
    // use CLOCK_MONOTONIC for relative time
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);

    double elapsed_time = (t2.tv_sec) * 1000l;
    elapsed_time += (t2.tv_nsec) / (double)1000/(double)1000;

    double ret = elapsed_time;
    return ret;
}


int my_rand()
{
	static random_device rd;
	static mt19937 g(rd());
	static uniform_int_distribution<> dis(1, 1000 * 1000 * 1000);
	return dis(g);
}


vector<char *> make_buffers(int num,int segment_size)
{
	vector<char *> buffers(num);
	for(int i=0;i<num;i++)
	{
		buffers[i]=(char *)malloc(segment_size);
	}
	return buffers;
}
void split_copy(string &block, char *output[], int segment_size)
{
	assert(block.size()%segment_size==0);
	int num=block.size()/segment_size;
	for(int i=0;i<num;i++)
	{
		memcpy(output[i], &block[i*segment_size],segment_size);
	}
}

string combine_copy(vector<char *> buffers,int segment_size)
{
    string res;
    for(int i=0;i< (int)buffers.size();i++)
    {
        string tmp(segment_size,'a');
        memcpy(&tmp[0],buffers[i],segment_size);
        res+=tmp;
    }
    return res;
}
void one_test(int segment_size, int num_real,int num_fec)
{
	string input(segment_size*num_real,'a');
	for(int i=0;i<(int)input.size();i++)
	{
		input[i]=my_rand()%256;
	}

	auto buffers=make_buffers(num_real+num_fec, segment_size);
	split_copy(input, &buffers[0],segment_size);

	Encoder encoder;
	encoder.init(num_real,num_fec,segment_size);

    {
        double t1=get_cpu_time_ms();
        encoder.encode(&buffers[0],&buffers[num_real],segment_size);
        double t2=get_cpu_time_ms();
        fprintf(stderr,"encode_time=%f; ", (t2-t1));
    }


    auto pkt=make_buffers(num_real,segment_size);

	Decoder decoder;
	decoder.init(num_real,num_fec,segment_size);

    vector<int> shuffle;
    for(int i=0;i<num_real+num_fec;i++)
    {
        shuffle.push_back(i);
    }

    for(int i=0;i<(int)shuffle.size();i++)
    {
        swap(shuffle[i],shuffle[ my_rand()%shuffle.size()]);
    }

    {
        double t1=get_cpu_time_ms();

        int cnt=0;
        for(int i=num_fec+num_real-1;i>=0;i--)
        {
            //int idx=shuffle[i];
            int idx;
	    if(g_use_shuffle)
		    idx=shuffle[i];
	    else idx=i;

            cnt++;
            decoder.feed(idx,buffers[idx],segment_size);
            if(decoder.try_decode(&pkt[0],segment_size)==0 )
            {  
                fprintf(stderr,"decoded with %d packets; ",cnt);
                break; 
            }
            //pkt.push_back(buffers[i]);
            //index.push_back(i);
        }
        double t2=get_cpu_time_ms();
        fprintf(stderr,"decode_time=%f\n", (t2-t1));
    }


    string output;
    output=combine_copy(pkt,segment_size);

    //printf("<%d %d>\n",(int)input.size(),(int)output.size());


    /*
    for(int i=0;i<input.size();i++)
    {
        printf("[%02x]",(int)(unsigned char)input[i]);
    }
    printf("\n");

    for(int i=0;i<output.size();i++)
    {
        printf("[%02x]",(int)(unsigned char)output[i]);
    }
    printf("\n");*/

    assert(input.size()==output.size());
    assert(input==output);

}

int wirehair_test(void)
{
    fprintf(stderr,"segment_size=%d num_real=%d,num_fec=%d\n",g_segment_size,g_num_real,g_num_fec);
    for(int i=0;i<g_iterations;i++)
    {
	if(g_use_sleep) usleep(1000*1000);
        one_test(g_segment_size,g_num_real,g_num_fec);
    }
    return 0;
}
