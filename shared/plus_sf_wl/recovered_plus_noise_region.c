#include "recovered_plus_noise_region.h"
#include "sf_wl_math.inc"
typedef union sf_number {uint32_t u;float f;} sf_number;
static float number(uint32_t u){sf_number v;v.u=u;return v.f;}
static float stored(double x){volatile float y=(float)x;return y;}
static double add(double x,double y){volatile double z=x+y;return z;}
static double mul(double x,double y){volatile double z=x*y;return z;}
int sony_recovered_plus_noise_regions(const sony_plus_noise_region_config *g,
    sony_plus_noise_region_channel c[2]){
    if(!g||!c||g->channels<1||g->channels>2||g->units>32||g->subbands<1||g->subbands>16||g->profile>31)return -1;
    for(uint32_t ch=0;ch<g->channels;++ch)for(uint32_t q=0;q<g->units;++q){
        sf_number v;v.f=c[ch].metric[q];
        if((v.u&0x80000000u)||(v.u&0x7f800000u)==0x7f800000u||
           c[ch].integer_bias[q]<0||c[ch].integer_bias[q]>32767||c[ch].word_length[q]<0||c[ch].word_length[q]>7)return -1;
    }
    const uint32_t regions=sf_wl_region_count[g->subbands]+1u;
    const float factor=number(sf_wl_numeric_constants[((g->profile>=15?0x456680u:0x4565a8u)-0x4565a0u)>>2]);
    const int32_t minimum=g->profile>=15?5:g->profile>=13?3:0;
    for(uint32_t ch=0;ch<g->channels;++ch){
        for(uint32_t i=0;i<5;++i)c[ch].region[i]=15;
        if(!c[ch].active)continue;
        float metric[5]={0},bias[5]={0},wl[5]={0};uint32_t count[5]={0};
        for(uint32_t q=0;q<g->units;++q){
            const uint32_t region=sf_wl_qu_noise_region[q];uint32_t metric_ch=ch,integer_ch=ch;
            if(g->channels==2&&g->stereo_flags[sf_wl_qu_subband[q]]){
                metric_ch=1u-ch;integer_ch=metric_ch;
                if(ch==0&&c[1].word_length[q]==0&&c[0].word_length[q]>0&&c[1].scale_factor[q]==0)integer_ch=0;
            }else if(ch==1&&c[1].word_length[q]==0&&c[0].word_length[q]>0&&c[1].scale_factor[q]==0)integer_ch=0;
            metric[region]=stored(add(metric[region],c[metric_ch].metric[q]));
            bias[region]=stored(add(bias[region],c[integer_ch].integer_bias[q]));
            wl[region]=stored(add(wl[region],c[integer_ch].word_length[q]));++count[region];
        }
        for(uint32_t region=0;region<regions;++region){
            if(metric[region]>0)metric[region]=stored((double)metric[region]/count[region]);
            if(bias[region]>0)bias[region]=stored((double)bias[region]/count[region]);
            if(wl[region]>0)wl[region]=stored((double)wl[region]/count[region]);
            if(!region)continue;
            for(int32_t i=15;i>=0;--i)if(metric[region]>number(sf_wl_noise_levels[i])){c[ch].region[region]=i;break;}
            if(c[ch].region[region]<15){
                c[ch].region[region]-=(int32_t)mul(factor,bias[region]);
                if(g->profile<=13&&wl[region]<number(sf_wl_noise_wl_threshold[region]))--c[ch].region[region];
                if(c[ch].region[region]<minimum)c[ch].region[region]=minimum;
            }
        }
        for(uint32_t region=0;region<regions;++region)if(c[ch].region[region]<15){
            c[ch].region[region]+=(int32_t)(g->profile>=15?sf_wl_noise_bias_high[region]:sf_wl_noise_bias_low[region]);
            if(c[ch].region[region]>15)c[ch].region[region]=15;
            if(c[ch].region[region]<0)c[ch].region[region]=0;
        }
    }
    return 0;
}
