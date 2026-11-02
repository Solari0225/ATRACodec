/* Independently reconstructed from Sony-executed numerical function40d24d.
 * Provenance and typed actual/native/portable checks: research/plus_decode.
 * No external codec implementation is used. */
#include "plus_imdct_float.h"
#include "plus_imdct_tables.h"

static float add32(float a, float b) { volatile float r = a+b; return r; }
static float sub32(float a, float b) { volatile float r = a-b; return r; }
static float mul32(float a, float b) { volatile float r = a*b; return r; }

void atracodec_plus_imdct_float(float spectrum[128], float output[128],
                              float history[64], int window_flag,
                              float gain_scale, int reverse_flag)
{
    unsigned i, width, group, coefficient = 1;
    const float *window = window_flag ? plus_imdct_window1 : plus_imdct_window0;
    for(i=0;i<64;++i) {
        float low=spectrum[i], high=spectrum[127-i];
        if(reverse_flag) {
            spectrum[i]=sub32(high,low);
            spectrum[127-i]=low;
        } else spectrum[i]=sub32(low,high);
    }
    for(i=0;i<64;++i) {
        float low=spectrum[i];
        float high=mul32(spectrum[i+64],plus_imdct_butterfly32[0]);
        spectrum[i]=add32(low,high);
        spectrum[i+64]=sub32(low,high);
    }
    for(width=32;width>=4;width/=2) {
        for(group=0;group<64/width;++group) {
            float difference[32];
            float *low=spectrum+group*width*2;
            float *high=low+width;
            float factor=plus_imdct_butterfly32[coefficient++];
            for(i=0;i<width;++i) difference[i]=sub32(low[i],high[width-1-i]);
            for(i=0;i<width;++i) {
                float product=mul32(high[i],factor);
                low[i]=add32(difference[i],product);
                high[i]=sub32(difference[i],product);
            }
        }
    }
    for(group=0;group<32;++group) {
        float *v=spectrum+group*4;
        float a=sub32(v[0],v[3]), b=sub32(v[1],v[2]);
        /* x87 PC=53 actual at40d24d. The two products are not rounded to
         * binary32 before the four final stores. */
        volatile double p=(double)v[2]*(double)plus_imdct_fourpoint32[group];
        volatile double q=(double)v[3]*(double)plus_imdct_fourpoint32[group];
        v[0]=(float)((double)a+p);
        v[1]=(float)((double)b+q);
        v[2]=(float)((double)a-p);
        v[3]=(float)((double)b-q);
    }
    for(group=0;group<8;++group) {
        float newer[2][4], present[2][4], old[2][4];
        const float *v=spectrum+group*16;
        const float *pre=plus_imdct_prerotation128+group*16;
        const float *post=plus_imdct_postrotation64+group*8;
        const float *win=window+group*16;
        const unsigned char *permutation=plus_imdct_permutation64+group*8;
        unsigned pair,lane;
        for(pair=0;pair<2;++pair) for(lane=0;lane<4;++lane) {
            float first=v[lane*4+pair*2], second=v[lane*4+pair*2+1];
            float difference=sub32(first,second);
            float rotated=mul32(second,post[pair*4+lane]);
            float neg=sub32(difference,rotated);
            float pos=add32(difference,rotated);
            newer[pair][lane]=mul32(pos,pre[8+pair*4+lane]);
            present[pair][lane]=mul32(mul32(neg,pre[pair*4+lane]),gain_scale);
            old[pair][lane]=-history[permutation[lane*2+pair]];
        }
        for(pair=0;pair<2;++pair) for(lane=0;lane<4;++lane) {
            unsigned index=permutation[lane*2+pair];
            float a=win[pair*8+lane], b=win[pair*8+4+lane];
            history[index]=newer[pair][lane];
            output[63-index]=add32(mul32(old[pair][lane],a),mul32(present[pair][lane],b));
            output[64+index]=sub32(mul32(old[pair][lane],b),mul32(present[pair][lane],a));
        }
    }
}
