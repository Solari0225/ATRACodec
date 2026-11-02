#include "recovered_peak_lanes.h"
#include <stdint.h>
#include <string.h>

/* After clearing the sign bit, positive IEEE float order equals uint32 order.
   SSE MAXPS returns its second operand on unordered or equal comparisons.
   Working in bits also preserves signalling-NaN payloads without arithmetic. */
static uint32_t maxps_positive_bits(uint32_t a,uint32_t b){
    if(a>0x7f800000u||b>0x7f800000u)return b;
    return a>b?a:b;
}
int sony_recovered_peak_lanes_f32(const float input[1024],float output[128]){
    uint32_t source[1024],result[128];
    if(!input||!output)return -1;
    memcpy(source,input,sizeof(source));
    for(unsigned window=0;window<32;++window)for(unsigned lane=0;lane<4;++lane){
        uint32_t x[8];
        for(unsigned k=0;k<8;++k)x[k]=source[window*32+lane+4*k]&0x7fffffffu;
        uint32_t a=maxps_positive_bits(x[0],x[1]);
        uint32_t b=maxps_positive_bits(x[2],x[3]);
        uint32_t c=maxps_positive_bits(x[4],x[5]);
        uint32_t d=maxps_positive_bits(x[6],x[7]);
        uint32_t e=maxps_positive_bits(a,b);
        uint32_t f=maxps_positive_bits(e,c);
        result[lane*32+window]=maxps_positive_bits(f,d);
    }
    memcpy(output,result,sizeof(result));return 0;
}
