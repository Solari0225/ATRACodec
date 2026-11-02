#include "recovered_atrac3_mdct.h"
#include <string.h>
typedef struct {float lane[4];} vector4;
static float binary32(float value){volatile float rounded=value;return rounded;}
static vector4 load4(const float* p){vector4 v;memcpy(v.lane,p,sizeof(v));return v;}
static void store4(float* p,vector4 v){memcpy(p,v.lane,sizeof(v));}
static vector4 add4(vector4 a,vector4 b){for(unsigned i=0;i<4;++i)a.lane[i]=binary32(a.lane[i]+b.lane[i]);return a;}
static vector4 sub4(vector4 a,vector4 b){for(unsigned i=0;i<4;++i)a.lane[i]=binary32(a.lane[i]-b.lane[i]);return a;}
static vector4 mul4(vector4 a,vector4 b){for(unsigned i=0;i<4;++i)a.lane[i]=binary32(a.lane[i]*b.lane[i]);return a;}
int sony_recovered_atrac3_mdct(sony_atrac3_mdct_state* state,
                              const sony_atrac3_mdct_tables* tables){
    if(!state||!tables)return -1;
    for(unsigned i=0;i<128;++i)if(tables->permutation[i]>=128)return -1;
    for(unsigned i=0;i<4;++i)if(state->gain_index[i]>=16)return -1;
    vector4 temporary[256],gain;
    for(unsigned lane=0;lane<4;++lane)gain.lane[lane]=tables->gain_scale[state->gain_index[lane]];
    for(unsigned i=0;i<128;++i){
        unsigned position=tables->permutation[i];
        vector4 tail=mul4(load4(state->samples+2048+4*i),gain);
        vector4 high=load4(state->samples+4*(384+position));
        vector4 low=load4(state->samples+4*(383-position));
        const float* c=tables->fold+16*position;
        vector4 difference=sub4(mul4(high,load4(c)),low);
        vector4 next_overlap=mul4(load4(c+4),difference);
        store4(state->samples+2048+4*i,next_overlap);
        vector4 sum=mul4(add4(mul4(low,load4(c)),high),load4(c+8));
        vector4 first=add4(sum,tail);
        vector4 second=mul4(load4(c+12),sub4(sum,tail));
        temporary[2*i]=add4(first,second);temporary[2*i+1]=second;
    }
    memcpy(state->samples+1024,state->samples,1024*sizeof(float));
    for(unsigned i=0;i<64;++i){
        unsigned first=4*i;vector4 c=load4(tables->butterfly+8*i);
        vector4 a=temporary[first],b=temporary[first+1],d=temporary[first+2],e=temporary[first+3];
        vector4 odd=mul4(c,sub4(b,e));
        vector4 even=mul4(c,sub4(a,d));
        temporary[first]=add4(add4(a,d),odd);
        temporary[first+1]=add4(add4(b,e),even);
        temporary[first+2]=even;temporary[first+3]=odd;
    }
    for(unsigned stride=4;stride<128;stride*=2){
        int phase=(int)stride/2-128;
        for(unsigned block=0;block<256;block+=2*stride,phase+=(int)stride){
            /* These negative offsets select the interleaved coefficient set
               immediately before the folding table in the original image. */
            vector4 c=load4(tables->butterfly+4*(127+phase));
            for(unsigned j=0;j<stride;j+=2){
                unsigned first=block+j;
                vector4 a=temporary[first],b=temporary[first+1];
                vector4 d=temporary[first+stride],e=temporary[first+stride+1];
                temporary[first+stride]=mul4(c,sub4(a,d));
                temporary[first+stride+1]=mul4(c,sub4(b,e));
                temporary[first]=add4(a,d);temporary[first+1]=add4(b,e);
            }
            for(unsigned j=0;j<stride/4;++j){
                unsigned first=block+4*j,last=block+2*stride-1-4*j;
                vector4 a=add4(temporary[last-1],temporary[first+1]);
                vector4 b=add4(temporary[last-2],temporary[first+2]);
                vector4 d=add4(temporary[last-3],temporary[first+3]);
                vector4 e=add4(temporary[last],temporary[first]);
                temporary[first]=e;temporary[first+1]=a;temporary[first+2]=b;temporary[first+3]=d;
            }
        }
    }
    vector4 scale=load4(tables->final_scale);
    for(unsigned i=0;i<128;++i){
        vector4 low=temporary[i],high=temporary[i+128];
        temporary[i+128]=mul4(scale,sub4(low,high));temporary[i]=add4(low,high);
    }
    for(unsigned i=0;i<128;++i){
        vector4 high=temporary[255-i],sum=add4(temporary[i],high);
        state->samples[i]=sum.lane[0];state->samples[255-i]=high.lane[0];
        state->samples[256+i]=high.lane[1];state->samples[511-i]=sum.lane[1];
        state->samples[512+i]=sum.lane[2];state->samples[767-i]=high.lane[2];
        state->samples[768+i]=high.lane[3];state->samples[1023-i]=sum.lane[3];
    }
    return 0;
}
