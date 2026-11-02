#include "recovered_atrac3_exponents.h"
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24 &&
               FLT_MAX_EXP==128, "IEEE binary32 required");
static uint32_t magnitude_word(float value)
{
    union { float value; uint32_t bits; } data;
    data.value=value;return data.bits<<1;
}
uint32_t sony_recovered_atrac3_exponent4(const float spectrum[4])
{
        uint32_t peak=magnitude_word(spectrum[0]);
        for(uint32_t lane=1;lane<4;++lane){
            const uint32_t value=magnitude_word(spectrum[lane]);
            if(value>peak)peak=value;
        }
        uint32_t estimate=(peak>>24)*3-UINT32_C(0x16c);
        const uint32_t fraction=peak&UINT32_C(0x00ffffff);
        if(fraction>=UINT32_C(0x965fea))++estimate;
        if(fraction<UINT32_C(0x428a30))--estimate;
        return estimate>63?0:estimate;
}
void sony_recovered_atrac3_exponents(const float spectrum[1024],
                                    uint32_t estimates[256])
{
    for(uint32_t group=0;group<256;++group)
        estimates[group]=sony_recovered_atrac3_exponent4(spectrum+4*group);
}
