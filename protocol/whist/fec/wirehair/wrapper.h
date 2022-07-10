#include "whist/utils/clock.h"
extern "C"
{
#include <whist/fec/wirehair/wirehair.h>
}

#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <string.h>
using namespace std;


const int verbose_log =0;
struct Encoder
{
	int num_real_;
	int num_fec_;
	WirehairCodec code;
	void init(int num_real,int num_fec,int max_sz)
	{
		wirehair_init();
		//assert(cauchy_256_init()==0);

		this->num_fec_=num_fec;
		this->num_real_=num_real;

		//wirehair_encoder_create(code );
	}
	void encode(char *src[], char *dst[], int sz)
	{
		char buf[num_real_*sz];
		for(int i=0;i<num_real_;i++)
		{
			memcpy(&buf[i*sz], src[i],sz);
		}

		{
		    double t1=get_timestamp_ms();
		    code=wirehair_encoder_create(0, buf, num_real_*sz, sz);
		    double t2=get_timestamp_ms();
		    fprintf(stderr,"<encoder create cost %f>\n", (t2-t1));
		}
		unsigned int out_sz=111;
		for(int i=0;i<num_fec_;i++)
		{
		    double t1= get_timestamp_ms();
		    int r=wirehair_encode(code,num_real_+i,dst[i],sz,&out_sz);
		    double t2= get_timestamp_ms();
		    if(verbose_log)
		    fprintf(stderr,"<encode idx=%d cost %f>", num_real_+i, t2-t1);
//printf("<%d %d %d>\n",r,sz,out_sz);
			//assert(out_sz==sz);
		}
		fprintf(stderr,"\n");
	}
};

struct Decoder
{
	int num_real_;
	int num_fec_;
	WirehairCodec code;

	int ok=0;

	void init(int num_real,int num_fec,int max_sz)
	{
		this->num_fec_=num_fec;
		this->num_real_=num_real;
		ok=0;
		code=wirehair_decoder_create(0,num_real*max_sz,max_sz);
	}

	void feed(int index,char *pkt,int sz)
	{
		int r=wirehair_decode(code,index,pkt,sz);
		if(r==0) ok=1;
	}

	int try_decode(char *pkt[],int sz)
	{
		if(ok==0) return -1;
		char buf[num_real_*sz];

		double t1=get_timestamp_ms();
		wirehair_recover(code,buf,num_real_*sz);
		double t2=get_timestamp_ms();
		fprintf(stderr,"\ndecode recover cost= %f\n", t2-t1);
		for(int i=0;i<num_real_;i++)
		{
			memcpy(pkt[i],&buf[i*sz],sz);
		}
		double t3=get_timestamp_ms();
		fprintf(stderr,"\ndecode memcpy cost= %f\n", t3-t2);
		return 0;
	}
};
