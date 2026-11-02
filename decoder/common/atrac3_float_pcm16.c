/* Independently recovered from Sony467150..467200 numeric loop.
 * The larger wrapper's core parsing, stream counters and stack management
 * are outside this leaf. No original executable code is used here.
 */
#include "atrac3_float_pcm16.h"
#include <float.h>
#include <string.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24 && FLT_MAX_EXP==128,"IEEEbinary32 required");
static int16_t convert(float value){
    if(value>32767.0f)value=32767.0f;
    if(value<-32767.0f)value=-32767.0f;
    /* Addition followed by a binary32 store rounds to an integer at this
     * exponent. Low16bits are the original two's-complement PCM result. */
    const float biased=value+12582912.0f;uint32_t bits;
    memcpy(&bits,&biased,sizeof(bits));const uint32_t u=bits&65535u;
    return u<32768u?(int16_t)u:(int16_t)((int32_t)u-65536);
}
int atracodec_atrac3_float_pcm16(const float *left,const float *right,int16_t *output){
    if(!left||!right||!output)return -1;
    for(unsigned i=0;i<1024;++i){output[2*i]=convert(left[i]);output[2*i+1]=convert(right[i]);}
    return 0;
}
