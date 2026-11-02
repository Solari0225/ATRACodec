/* Independent equivalent of Sony3da620/3daba0 numeric conversion.
 * Integer decomposition makes binary32 nearest-even conversion identical
 * on Windows/macOS Intel/ARM, without an x87 or SIMD dependency.
 */
#include "plus_float_pcm16.h"
#include <string.h>
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24,"IEEEbinary32 required");
static int16_t convert(float value){
    uint32_t bits;memcpy(&bits,&value,4);const uint32_t exponent=(bits>>23)&255u;
    uint32_t magnitude=0;int32_t integer;
    if(exponent>=158u)integer=INT32_MIN; /* NaN/Inf and FISTP32 overflow too. */
    else{
        const uint32_t fraction=bits&0x7fffffu;
        if(exponent==126u)magnitude=fraction!=0; /* Half ties to zero. */
        else if(exponent>=127u){
            const uint32_t significand=fraction|0x800000u;
            if(exponent>=150u)magnitude=significand<<(exponent-150u);
            else{
                const unsigned shift=150u-exponent;
                magnitude=significand>>shift;
                const uint32_t remainder=significand&((1u<<shift)-1u),half=1u<<(shift-1u);
                if(remainder>half||(remainder==half&&(magnitude&1u)))++magnitude;
            }
        }
        integer=(bits>>31)?-(int32_t)magnitude:(int32_t)magnitude;
    }
    if(integer>32767)integer=32767;
    else if(integer<-32768)integer=-32768;
    return(int16_t)integer;
}
int atracodec_plus_float_pcm16(const float *left,const float *right,int16_t *output){
    if(!left||!right||!output)return -1;
    for(unsigned i=0;i<2048;++i){output[2*i]=convert(left[i]);output[2*i+1]=convert(right[i]);}
    return 0;
}
