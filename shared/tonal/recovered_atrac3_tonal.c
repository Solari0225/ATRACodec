#include "recovered_atrac3_tonal.h"
#include "../exponents/recovered_atrac3_exponents.h"
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_MANT_DIG==24 && sizeof(double)==8 &&
               DBL_MANT_DIG==53,"IEEE binary32/binary64 required");
#include "tonal_tables.inc"
static float number(uint32_t bits)
{
    union { uint32_t bits; float value; } data;
    data.bits=bits;return data.value;
}
static float scale_for(uint32_t wl,uint32_t sf)
{
    return number(tonal_scales[3*wl-sf%3]-((sf/3)<<23));
}
static int32_t rounded_integer(double scaled)
{
    union { uint32_t bits; float value; } data;
    volatile double shifted=scaled+(double)number(TONAL_MAGIC);
    data.value=(float)shifted;
    const uint32_t low=data.bits&65535;
    return low<32768?(int32_t)low:(int32_t)low-65536;
}
int sony_recovered_atrac3_select_tonal_sf(const float input[4],uint32_t wl,
                                        uint32_t count,uint32_t *sf)
{
    if(!input||!sf||*sf>63||wl<2||wl>7||!count||count>4)return -1;
    const uint32_t coarse=sony_recovered_atrac3_exponent4(input);
    int32_t candidate=(int32_t)(coarse+3>64?64:coarse+3)-1;
    volatile double best=(double)number(TONAL_INITIAL_ERROR);
    for(;candidate>=(int32_t)coarse;--candidate){
        const float scale=scale_for(wl,(uint32_t)candidate);
        float maximum_error=0;
        int32_t combined=0;
        for(uint32_t i=0;i<count;++i){
            volatile double scaled=(double)scale*(double)input[i];
            const int32_t q=rounded_integer(scaled);combined|=q;
            volatile double difference=scaled-(double)q;
            const double magnitude=difference<0?-difference:difference;
            if((double)maximum_error<magnitude)maximum_error=(float)magnitude;
        }
        volatile double limit=(double)scale*best;
        if(limit>(double)maximum_error){
            best=(double)maximum_error/(double)scale;
            *sf=(uint32_t)candidate;
            while(!(combined&1)&&*sf<61){
                /* Defined arithmetic right shift, preserving negative q. */
                combined=combined>=0?combined/2:-1-(int32_t)((uint32_t)(-1-combined)/2);
                *sf+=3;
            }
        }
    }
    return 0;
}
int32_t sony_recovered_atrac3_quantize_tonal(float input[4],
    sony_atrac3_tonal_component *component,uint32_t wl,uint32_t count)
{
    if(!component)return -1;
    const int status=sony_recovered_atrac3_select_tonal_sf(input,wl,count,
                                                        &component->scale_factor);
    if(status)return status;
    const float scale=scale_for(wl,component->scale_factor);
    volatile double reciprocal=(double)number(TONAL_ONE)/(double)scale;
    const float inverse=(float)reciprocal;
    uint32_t bits=12;
    for(uint32_t i=0;i<count;++i){
        volatile double scaled=(double)scale*(double)input[i];
        const int32_t q=rounded_integer(scaled);
        const uint32_t symbol=(uint32_t)q&tonal_masks[wl];
        volatile double reconstruction=(double)q*(double)inverse;
        volatile double residual=(double)input[i]-reconstruction;
        input[i]=(float)residual;component->symbol[i]=symbol;
        bits+=tonal_lengths[wl][symbol];
    }
    return (int32_t)bits;
}
int32_t sony_recovered_atrac3_collect_tonal_peaks(const float spectrum[1024],
    uint32_t sf,uint32_t begin,uint32_t end,uint32_t *positions,uint32_t capacity)
{
    if(!spectrum||!positions||sf>63||begin>=end||end>1024)return -1;
    const uint32_t threshold=tonal_thresholds[sf]<<1;
    uint32_t count=0;
    for(uint32_t i=begin;i<end;++i){
        union {float value;uint32_t bits;} data;
        data.value=spectrum[i];
        if((data.bits<<1)>=threshold){
            if(count==capacity)return -1;
            positions[count++]=i;i+=3;
        }
    }
    return (int32_t)count;
}
int sony_recovered_atrac3_restore_tonal(float spectrum[1024],
    const sony_atrac3_tonal_component *component,uint32_t wl,uint32_t count)
{
    if(!spectrum||!component||wl>7||!tonal_fixed_widths[wl]||!count||count>4||
       component->scale_factor>63||component->position>1024-count)return -1;
    const uint32_t width=tonal_fixed_widths[wl],range=1u<<width;
    for(uint32_t i=0;i<count;++i)if(component->symbol[i]>=range)return -1;
    const float scale=scale_for(wl,component->scale_factor);
    volatile double inverse=(double)number(TONAL_ONE)/(double)scale;
    for(uint32_t i=count;i-->0;){
        const uint32_t symbol=component->symbol[i];
        const int32_t q=symbol<range/2?(int32_t)symbol:(int32_t)symbol-(int32_t)range;
        volatile double reconstruction=(double)q*inverse;
        volatile double sum=(double)spectrum[component->position+i]+reconstruction;
        spectrum[component->position+i]=(float)sum;
    }
    return 0;
}
