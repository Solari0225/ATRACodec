#include "plus_power_float.h"
#include "plus_power_float_tables.inc"

static int low_signed16(int32_t value)
{
    uint32_t u=(uint32_t)value&65535u;
    return u<32768u?(int)u:(int)u-65536;
}
static int valid_gain(const atracodec_plus_gain*g)
{
    unsigned i;
    if(g->count>7)return 0;
    for(i=0;i<g->count;++i)if(g->levels[i]>15)return 0;
    return 1;
}
/* Sony403550: the verified16-entry signed gain-exponent table is level-6.
 * The maximum is nonnegative and at most12 over legal gain syntax. */
static unsigned gain_exponent(const atracodec_plus_gain*current,const atracodec_plus_gain*previous)
{
    int base=current->count?6-(int)current->levels[0]:0,maximum=0;
    unsigned i;
    for(i=0;i<previous->count;++i) {
        int candidate=base+6-(int)previous->levels[i];
        if(maximum<candidate)maximum=candidate;
    }
    for(i=0;i<current->count;++i) {
        int candidate=6-(int)current->levels[i];
        if(maximum<candidate)maximum=candidate;
    }
    return (unsigned)maximum;
}
int atracodec_plus_apply_power_float(const atracodec_plus_power_config*c,float spectrum[2048])
{
    unsigned begin,end,q,exponent;
    float amplitude,band_scale;
    if(!c||!spectrum||c->band>15||c->power_level>15)return -1;
    amplitude=plus_power_levels[c->power_level];
    if(!(amplitude>0.0f))return 0;
    if(c->seed16>1020||!valid_gain(&c->reference_current_gain)||!valid_gain(&c->reference_previous_gain))return -1;
    begin=plus_power_first_qu[c->band];end=plus_power_qu_band[c->band+1];
    for(q=begin;q<end;++q) {
        int wl=low_signed16(c->target_word_length[q]);
        if(wl>0) {
            int sf=low_signed16(c->target_scale_factor[q]);
            if(wl>7||sf<0||sf>63)return -1;
        }
    }
    exponent=gain_exponent(&c->reference_current_gain,&c->reference_previous_gain);
    band_scale=(float)((double)amplitude/(double)(1u<<exponent));
    for(q=begin;q<end;++q) {
        int wl=low_signed16(c->target_word_length[q]);
        if(wl>0) {
            unsigned i,start=plus_power_qu_start[q],count=plus_power_qu_start[q+1]-start;
            int sf=low_signed16(c->target_scale_factor[q]);
            double factor=(double)plus_power_scale_factor[sf]*(double)plus_power_word_factor[wl];
            float stored_factor;
            factor=factor*(double)band_scale;
            factor=factor/(double)(1u<<(unsigned)wl);
            stored_factor=(float)factor;
            /* Original resets its128-sample noise pointer for every QU. */
            for(i=0;i<count;++i) {
                double product=(double)stored_factor*(double)plus_power_noise[c->seed16+i];
                spectrum[start+i]=(float)(product+(double)spectrum[start+i]);
            }
        }
    }
    return 0;
}
