#include "recovered_plus_refine.h"
#include "energy_tables.inc"
typedef union refine_number {uint32_t u;float f;} refine_number;
static float number(uint32_t u){refine_number v;v.u=u;return v.f;}
static float constant(uint32_t offset){return number(energy_constant[(offset-0x4565a0u)/4u]);}
static double add(double a,double b){volatile double c=a+b;return c;}
static double mul(double a,double b){volatile double c=a*b;return c;}
static double divd(double a,double b){volatile double c=a/b;return c;}
static float stored(double a){volatile float b=(float)a;return b;}
static float fadd(float a,float b){volatile float c=a+b;return c;}
static float fmul(float a,float b){volatile float c=a*b;return c;}
static int16_t word(int32_t x){return (int16_t)(uint16_t)x;}
static void energies(const float *quantized,const float *original,uint32_t count,uint32_t simd,double *qe,double *oe){
    double a=0,b=0;
    if(simd){float qa[4]={0,0,0,0},oa[4]={0,0,0,0};for(uint32_t i=0;i<count;i+=4)for(uint32_t j=0;j<4;++j){qa[j]=fadd(qa[j],fmul(quantized[i+j],quantized[i+j]));oa[j]=fadd(oa[j],fmul(original[i+j],original[i+j]));}a=fadd(fadd(qa[0],qa[1]),fadd(qa[2],qa[3]));b=fadd(fadd(oa[0],oa[1]),fadd(oa[2],oa[3]));}
    else for(uint32_t i=count;i>0;i-=4)for(uint32_t j=i-4;j<i;++j){a=add(a,mul(quantized[j],quantized[j]));b=add(b,mul(original[j],original[j]));}
    *qe=a;*oe=b;
}
int sony_recovered_plus_refine_scales(const sony_plus_refine_config *g,sony_plus_refine_channel c[2],sony_plus_refine_shared *s,int32_t allowed){
    if(!g||!c||!s||g->sf.channels<1||g->sf.channels>2||g->sf.units>32||g->units>32||g->profile>31||g->subbands>16||g->noise.channels<1||g->noise.channels>2)return -1;
    int32_t scales[2][32];sony_plus_energy_noise_channel nc[2];
    for(uint32_t ch=0;ch<2;++ch){nc[ch]=c[ch].noise;for(uint32_t q=0;q<32;++q){scales[ch][q]=c[ch].sf.coarse[q];if(scales[ch][q]<0||scales[ch][q]>63||c[ch].sf.word_length[q]<0||c[ch].sf.word_length[q]>7)return -1;}}
    uint16_t seeds[16]={0};if(sony_recovered_plus_noise_seeds(g->sf.channels,g->sf.units,g->subbands,scales,seeds))return -1;
    sony_plus_energy_noise_cache cache={-1,{0}};float paired[128]={0};
    const uint32_t begin=energy_profile_start[g->profile];
    for(uint32_t q=begin;q<g->units;++q)for(uint32_t ch=0;ch<g->sf.channels;++ch){
        const uint32_t reference=ch==1&&g->stereo_units[q]==1?0:ch;
        const int32_t wl=c[reference].sf.word_length[q];if(wl<=0)continue;
        const uint32_t count=energy_qu_count[q],start=energy_qu_start[q];float temporary[128],*quantized;
        if(ch==reference){for(uint32_t i=0;i<count;++i)temporary[i]=(float)c[ch].quantized[start+i];quantized=temporary;if(ch==0&&g->stereo_units[q]==1)for(uint32_t i=0;i<count;++i)paired[i]=temporary[i];}
        else quantized=paired;
        int32_t *coarse=c[ch].sf.coarse+q;const int32_t original=*coarse;if(original<=0)continue;
        if(c[ch].noise_enabled&&sony_recovered_plus_add_noise(&g->noise,nc,seeds[energy_qu_band[q]],ch,q,(uint32_t)wl,quantized,&cache,g->simd))return -1;
        const float scale=number(energy_scale[*coarse]);
        const float threshold=stored(mul(scale,number(energy_inverse_quant[wl])));
        double qe,oe;energies(quantized,c[ch].spectrum+start,count,g->simd,&qe,&oe);
        const float target=stored(mul(mul(scale,scale),oe));
        const double measured=mul(mul(qe,threshold),threshold);if(measured==0||target==0)continue;
        const float peak=c[reference].peakiness[q];float l1;
        if(sony_recovered_plus_absolute_ratio(quantized,scale,count,g->simd,&l1)<0)return -1;
        const float ratio=stored(divd(l1,mul(peak,threshold)));double current=stored(measured);
        if(current>=target){
            if(peak>=constant(0x45661c)&&ratio>=constant(0x4565a8))continue;
            while(current>target&&*coarse>0){current=mul(current,constant(0x456674));--*coarse;}
            if(current<mul(target,constant(0x45667c)))++*coarse;
        }else{
            while(current<target&&*coarse<63){current=mul(current,constant(0x456670));++*coarse;}
            if(current>mul(target,constant(0x456678)))--*coarse;
            if(peak>constant(0x456640)){
                if(ratio>constant(0x456654)||ratio<constant(0x456658))--*coarse;
            }else if(ratio>constant(0x456640))--*coarse;
            if(*coarse<original)*coarse=original;else if(*coarse-original>5)*coarse=original+5;
        }
    }
    sony_plus_sf_choice_channel sf[2]={c[0].sf,c[1].sf};int32_t bits;
    if(sony_recovered_plus_choose_scale_factors(&g->sf,sf,s->enabled,&bits))return -1;
    if(bits-s->sf_bits<=allowed){const int16_t old=s->overhead;s->overhead=word(s->overhead-s->sf_bits+bits);s->sf_bits=word(bits);s->total=word(s->total-old+s->overhead);}
    else{
        for(uint32_t ch=0;ch<g->sf.channels;++ch)for(uint32_t q=begin;q<g->units;++q)sf[ch].coarse[q]=scales[ch][q];
        if(sony_recovered_plus_choose_scale_factors(&g->sf,sf,s->enabled,&bits))return -1;
    }
    for(uint32_t ch=0;ch<g->sf.channels;++ch)c[ch].sf=sf[ch];
    return 0;
}
