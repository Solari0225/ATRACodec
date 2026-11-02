#include "recovered_plus_energy.h"
#include "energy_tables.inc"
typedef union energy_number {uint32_t u;float f;} energy_number;
static float number(uint32_t u){energy_number v;v.u=u;return v.f;}
static float absolute(float f){energy_number v;v.f=f;v.u&=0x7fffffffu;return v.f;}
static double add(double a,double b){volatile double v=a+b;return v;}
static double mul(double a,double b){volatile double v=a*b;return v;}
static double divd(double a,double b){volatile double v=a/b;return v;}
static float stored(double a){volatile float b=(float)a;return b;}
static float fadd(float a,float b){volatile float c=a+b;return c;}
static float fmul(float a,float b){volatile float c=a*b;return c;}
static int valid(const sony_plus_gain_points *p){if(!p||p->count>7)return 0;for(uint32_t i=0;i<p->count;++i)if(p->level[i]>15)return 0;return 1;}
int sony_recovered_plus_noise_seeds(uint32_t channels,uint32_t units,uint32_t subbands,
    const int32_t scales[2][32],uint16_t seeds[16]){
    if(!scales||!seeds||channels<1||channels>2||units>32||subbands>16)return -1;
    uint16_t sum=0;for(uint32_t ch=0;ch<channels;++ch)for(uint32_t q=0;q<units;++q)sum=(uint16_t)(sum+(uint16_t)scales[ch][q]);
    uint16_t seed=sum&0x3fc;for(uint32_t b=0;b<subbands;++b){seeds[b]=seed;seed=(seed+0x80)&0x3fc;}return 0;
}
int sony_recovered_plus_noise_gain(const sony_plus_gain_points *first,const sony_plus_gain_points *second,int32_t *out){
    if(!out||!valid(first)||!valid(second))return -1;
    const int16_t base=second->count?(int16_t)-energy_gain_exponents[second->level[0]]:0;int16_t maximum=0;
    for(uint32_t i=0;i<first->count;++i){const int16_t x=base-energy_gain_exponents[first->level[i]];if(maximum<x)maximum=x;}
    for(uint32_t i=0;i<second->count;++i){const int16_t x=-energy_gain_exponents[second->level[i]];if(maximum<x)maximum=x;}
    *out=maximum;return 0;
}
int sony_recovered_plus_absolute_ratio(const float *values,float factor,uint32_t count,uint32_t simd,float *out){
    if(!values||!out||count>2048||(count&3))return -1;
    double sum=0;
    if(simd){float lanes[4]={0,0,0,0};for(uint32_t i=0;i<count;i+=4)for(uint32_t j=0;j<4;++j)lanes[j]=fadd(lanes[j],absolute(values[i+j]));sum=fadd(fadd(lanes[0],lanes[1]),fadd(lanes[2],lanes[3]));}
    else{const uint32_t quarter=count>>2;for(uint32_t i=0;i<quarter;++i){double a=add(absolute(values[i+2*quarter]),absolute(values[i+3*quarter]));a=add(a,absolute(values[i+quarter]));a=add(a,absolute(values[i]));sum=add(sum,a);}}
    if(sum>0){*out=stored(divd(mul((double)count,factor),sum));return 1;}
    *out=0;return 0;
}
int sony_recovered_plus_add_noise(const sony_plus_energy_noise_config *g,const sony_plus_energy_noise_channel c[2],
    uint16_t seed,uint32_t ch,uint32_t unit,uint32_t wl,float values[128],sony_plus_energy_noise_cache *cache,uint32_t simd){
    if(!g||!c||!values||!cache||g->channels<1||g->channels>2||ch>=g->channels||unit>31||wl>7||seed>1020)return -1;
    const uint32_t band=energy_qu_band[unit],selected=g->channels==2&&g->stereo_band[band]?ch^1:ch;
    const int32_t region=c[selected].region[energy_band_region[band]];if(region<0||region>15)return -1;
    const float amplitude=number(energy_noise_scale[region]);if(unit<2||!(amplitude>0))return 0;
    const uint32_t count=energy_qu_count[unit];
    if(cache->band!=(int32_t)band){cache->band=(int32_t)band;for(uint32_t i=0;i<count;++i)cache->noise[i]=number(energy_noise_random[seed+i]);}
    int32_t gain;if(sony_recovered_plus_noise_gain(c[selected].previous+band,c[selected].current+band,&gain))return -1;
    const int32_t denominator=(int32_t)(1u<<((gain+wl)&31));const double scale=divd(amplitude,denominator);
    if(simd){const float f=stored(scale);for(uint32_t i=0;i<count;++i)values[i]=fadd(values[i],fmul(cache->noise[i],f));}
    else for(uint32_t i=0;i<count;++i)values[i]=stored(add(mul(scale,cache->noise[i]),values[i]));
    return 0;
}
int sony_recovered_plus_minimum_spectrum_cost(uint32_t channels,uint32_t units,const uint32_t banks[2],const int32_t words[2][32],int32_t *bits){
    if(!banks||!words||!bits||channels<1||channels>2||units>32)return -1;
    int32_t total=0;for(uint32_t ch=0;ch<channels;++ch){if(banks[ch]>1)return -1;for(uint32_t q=0;q<units;++q){const int32_t wl=words[ch][q];if(wl>7)return -1;if(wl>0)total+=energy_zero_cost[7*banks[ch]+wl]*(energy_qu_count[q]>>4);}}
    *bits=total;return 0;
}
