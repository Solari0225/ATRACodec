/* Independent Sony-derived three-stage float synthesis filter.
 * Recovered from the complete observed466200 graph. Native operation order
 * is retained: each24-term FIR is left-associated, then the two intermediate
 * bands are combined by the original four-lane transform. No native code or
 * CPU emulation is used. Compile without fast-math or FMA contraction.
 */
#include "atrac3_float_filter.h"
#include <stdint.h>
#include <string.h>
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24 && FLT_MAX_EXP==128,"IEEEbinary32 required");
typedef struct filter4 {float lane[4];} filter4;
_Static_assert(sizeof(filter4)==16,"Fourbinary32 lanes required");
static filter4 load4(const float *p){filter4 r;memcpy(&r,p,sizeof(r));return r;}
static void store4(float *p,filter4 r){memcpy(p,&r,sizeof(r));}
static filter4 mul4(filter4 a,filter4 b){
    filter4 r;for(unsigned i=0;i<4;++i)r.lane[i]=a.lane[i]*b.lane[i];return r;
}
static filter4 add4(filter4 a,filter4 b){
    filter4 r;for(unsigned i=0;i<4;++i)r.lane[i]=a.lane[i]+b.lane[i];return r;
}
static filter4 shuffle4(filter4 a,filter4 b,unsigned mask){
    filter4 r={{a.lane[mask&3],a.lane[(mask>>2)&3],b.lane[(mask>>4)&3],b.lane[(mask>>6)&3]}};return r;
}
#include "atrac3_float_filter_tables.inc"
static const uint8_t fir_index[3][24]={
    {51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74},
    {27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50},
    {3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26},
};
static filter4 dot24(const float *input,const filter4 weights[75],unsigned stage){
    const unsigned first=stage==2?0:46,stride=stage==2?2:4;
    filter4 sum=mul4(weights[fir_index[stage][0]],load4(input+first));
    for(unsigned i=1;i<24;++i)
        sum=add4(sum,mul4(weights[fir_index[stage][i]],load4(input+first+stride*i)));
    return sum;
}
int atracodec_atrac3_float_filter(float *buffer,size_t float_count){
    if(!buffer||float_count<1162)return -1;
    filter4 weights[75];memcpy(weights,filter_coefficients,sizeof(weights));
    for(size_t p=0;p<1024;p+=4){
        const filter4 a=dot24(buffer+p,weights,0),b=dot24(buffer+p,weights,1);
        const filter4 low=add4(shuffle4(a,b,0),mul4(shuffle4(a,b,85),weights[0]));
        const filter4 high=add4(mul4(shuffle4(a,b,170),weights[1]),mul4(shuffle4(a,b,255),weights[2]));
        store4(buffer+p+46,add4(low,high));
        store4(buffer+p,dot24(buffer+p,weights,2));
    }
    return 0;
}
