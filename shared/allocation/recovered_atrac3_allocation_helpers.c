#include "recovered_atrac3_allocation_helpers.h"
static const uint32_t unit_end[32]={8,16,24,32,40,48,56,64,80,96,112,128,
    144,160,176,192,224,256,288,320,352,384,416,448,480,512,576,640,704,768,896,1024};
static int32_t arithmetic_shift(int32_t value,uint32_t count)
{
    const uint32_t shift=count&31,bits=(uint32_t)value;
    if(!shift)return value;
    const uint32_t shifted=(bits>>shift)|((bits&UINT32_C(0x80000000))?
        UINT32_MAX<<(32-shift):0);
    return (int32_t)shifted;
}
int32_t sony_recovered_atrac3_clamp_word_length(int32_t metric,
                                              uint32_t shift,int32_t minimum)
{
    int32_t value=arithmetic_shift(metric,shift);
    if(value<minimum)value=minimum;
    return value>7?7:value;
}
int32_t sony_recovered_atrac3_gain_header_bits(uint32_t qu,
                                const uint32_t gain_counts[4],uint32_t *bands)
{
    if(!gain_counts||!bands||qu<1||qu>32)return -1;
    const uint32_t count=(unit_end[qu-1]+255)>>8;
    uint32_t points=0;
    for(uint32_t i=0;i<count;++i){if(gain_counts[i]>7)return -1;points+=gain_counts[i];}
    *bands=count;return (int32_t)(9*points+3*count);
}
int32_t sony_recovered_atrac3_trim_bands(uint32_t bands,
    uint32_t extended_header,uint32_t header_field,uint32_t group_count,
    const uint32_t gain_counts[4],const uint32_t flags[4][4],
    uint32_t *new_bands)
{
    if(!gain_counts||!flags||!new_bands||!bands||bands>4||group_count>4)return -1;
    *new_bands=bands;
    if(extended_header&&(header_field&7)!=7)return 0;
    uint32_t kept=bands;
    while(kept>1){
        const uint32_t b=kept-1;
        if(gain_counts[b])break;
        uint32_t present=0;
        for(uint32_t g=0;g<group_count;++g)present|=flags[g][b];
        if(present)break;
        --kept;
    }
    *new_bands=kept;return (int32_t)((bands-kept)*(group_count+3));
}
int32_t sony_recovered_atrac3_heuristic_cost(uint32_t unit,
    const uint32_t estimates[256],uint32_t wl,uint32_t sf)
{
    /* Exact numerical values captured in014, not fitted coefficients. */
    static const int32_t discount[8]={0,40,40,60,76,60,60,100};
    static const int32_t base[8]={0,15,20,25,29,35,45,55};
    static const int32_t sf_bias[8]={0,3,5,7,9,12,15,18};
    if(!estimates||unit>31||wl<1||wl>7||sf>63)return -1;
    const uint32_t start=unit?unit_end[unit-1]:0,end=unit_end[unit];
    int32_t result=base[wl]*(int32_t)(end-start)+60;
    const int32_t threshold=(int32_t)sf-sf_bias[wl];
    for(uint32_t i=start/4;i<end/4;++i){
        if(estimates[i]>63)return -1;
        if((int32_t)estimates[i]<threshold)result-=discount[wl];
    }
    return result;
}
