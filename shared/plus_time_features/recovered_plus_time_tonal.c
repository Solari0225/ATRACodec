#include "recovered_plus_time_tonal.h"
#include "recovered_plus_time_fft.h"
#include "../plus_analysis/independent_plus_math.h"
#include "time_math.inc"
#if defined(_MSC_VER) && !defined(__clang__)
#include <math.h>
#define time_sqrt sqrt
#else
#define time_sqrt __builtin_sqrt
#endif
static float spill(double x){volatile float f=(float)x;return f;}
static float four_sum(const float v[4]){return spill(((double)v[3]+v[2])+v[1]+v[0]);}
static float packed_dot(const float *a,const float *b,uint32_t n){
    float v[4]={0,0,0,0};for(uint32_t i=0;i<n;i+=4)for(uint32_t j=0;j<4;++j)v[j]=spill((double)v[j]+spill((double)a[i+j]*b[i+j]));
    const float x=spill((double)v[0]+v[1]),y=spill((double)v[2]+v[3]);return spill((double)x+y);
}
static float mixed_dot(const float *a,const float *b,uint32_t n){
    double v[3]={0,0,0};float last=0;
    for(uint32_t i=0;i<n;i+=4){for(uint32_t j=0;j<3;++j)v[j]+=(double)a[i+j]*b[i+j];last=spill((double)last+(double)a[i+3]*b[i+3]);}
    const float f[4]={spill(v[0]),spill(v[1]),spill(v[2]),last};return four_sum(f);
}
int sony_plus_time_dual_dot(const float *a,const float *b,const float *c,const float *d,float *first,float *second,uint32_t count,uint32_t backend){
    if(!a||!b||!c||!d||!first||!second||count>256)return -1;const uint32_t n=(count+3)&~3u;
    *first=backend?packed_dot(a,b,n):spill(sony_plus_time_dot(a,b,n,0));
    *second=backend?packed_dot(c,d,n):mixed_dot(c,d,n);return 0;
}
int sony_plus_time_triple_dot(const float *a,const float *b,const float *c,const float *d,const float *e,const float *f,float *first,float *second,float *third,uint32_t count,uint32_t backend){
    if(!e||!f||!third||sony_plus_time_dual_dot(a,b,c,d,first,second,count,backend))return -1;const uint32_t n=(count+3)&~3u;
    if(backend)*third=packed_dot(e,f,n);
    else {float v[4]={0,0,0,0};for(uint32_t i=0;i<n;i+=4)for(uint32_t j=0;j<4;++j)v[j]=spill((double)v[j]+(double)e[i+j]*f[i+j]);*third=four_sum(v);}return 0;
}
double sony_plus_time_fit_coefficients(const float input[256],int32_t frequency,float *sine_coefficient,float *cosine_coefficient,uint32_t backend){
    if(!input||!sine_coefficient||!cosine_coefficient||frequency<0||frequency>1024)return 0;
    float residual[256],sine[256],cosine[256],s,c;
    if(!frequency){double sum=0;
        if(backend){float v[4]={0,0,0,0};for(uint32_t i=0;i<256;i+=4)for(uint32_t j=0;j<4;++j)v[j]=spill((double)v[j]+input[i+j]);const float x=spill((double)v[0]+v[1]),y=spill((double)v[2]+v[3]);sum=spill((double)x+y);}
        else for(uint32_t i=0;i<256;i+=4){double x=(double)input[i]+input[i+1];x+=input[i+3];x+=input[i+2];sum+=x;}
        s=0;c=spill(sum*.00390625);
        for(uint32_t i=0;i<256;++i)residual[i]=spill((double)input[i]-c);
    }else{
        for(uint32_t i=0;i<256;++i){const uint32_t angle=i*(uint32_t)frequency;sine[i]=time_sine[angle&2047];cosine[i]=time_sine[(angle+512)&2047];}
        float ds,dc;sony_plus_time_dual_dot(input,sine,input,cosine,&ds,&dc,256,backend);
        if((frequency&3)!=0){
            const double inv=1.0/time_sine[(uint32_t)frequency];
            const double a=(256.0-(double)cosine[255]*time_sine[((uint32_t)frequency*256)&2047]*inv)*.5;
            const double b=(double)sine[255]*time_sine[((uint32_t)frequency*256)&2047]*inv*.5;
            const double d=256.0-a;const float det=spill(1.0/(b*b-d*a));
            s=spill((b*dc-d*ds)*det);c=spill((b*ds-a*dc)*det);
        }else{s=spill((double)ds*.0078125);c=spill((double)dc*.0078125);}
        for(uint32_t i=0;i<256;++i){double x=(double)s*sine[i],y=(double)c*cosine[i];
            if(backend){x=spill(x);y=spill(y);residual[i]=spill((double)input[i]-spill(x+y));}
            else residual[i]=spill((double)input[i]-(x+y));}
    }
    *sine_coefficient=s;*cosine_coefficient=c;return sony_plus_time_dot(residual,residual,256,backend);
}
uint32_t sony_plus_time_coefficient_phase(float sine_coefficient,float cosine_coefficient,int32_t frequency){
    const double x=sine_coefficient,y=cosine_coefficient;const double pi=0x1.921fb54442d18p+1;
    double angle;if(x!=0){angle=sony_plus_math_atan(y/x);if(x<0)angle+=y<0?-pi:pi;}
    else if(y!=0)angle=y<0?-pi*.5:pi*.5;
    else{union {float f;uint32_t u;} sign;sign.f=sine_coefficient;angle=(sign.u>>31)?pi:0;}
    const double shifted=angle*325.94932345220167+.5;int32_t rounded=(int32_t)shifted;if((double)rounded>shifted)--rounded;
    return ((uint32_t)rounded+((uint32_t)frequency<<7))&2047;
}
int sony_plus_time_refine(const float input[256],int32_t peak,float *amplitude,int32_t *phase,int32_t *frequency,uint32_t backend){
    if(!input||!amplitude||!phase||!frequency||peak<0||peak>128)return -1;
    const int32_t center=peak*8,start=center<4?0:center-4,end=center>1020?1024:center+4;
    float error[8],sine[8],cosine[8];int32_t indices[8];uint32_t count=0;
    for(int32_t f=start;f<end;f+=2){error[count]=spill(sony_plus_time_fit_coefficients(input,f,sine+count,cosine+count,backend));++count;}
    sony_plus_time_sort_power(error,indices,count);const uint32_t best=(uint32_t)indices[count-1];
    const int32_t step=indices[count-2]>(int32_t)best?1:-1;int32_t selected=start+(int32_t)best*2+step;
    float a,b;float candidate_error=spill(sony_plus_time_fit_coefficients(input,selected,&a,&b,backend));
    if(best==count-1){const int32_t f=start+(int32_t)best*2+1;float c,d;
        const double next=sony_plus_time_fit_coefficients(input,f,&c,&d,backend);
        if(next<candidate_error){candidate_error=spill(next);a=c;b=d;selected=f;}
    }
    if(candidate_error>=error[count-1]){a=sine[best];b=cosine[best];selected=start+(int32_t)best*2;}
    const double power=(double)b*b+(double)a*a;*amplitude=spill(time_sqrt(power));
    *phase=(int32_t)sony_plus_time_coefficient_phase(a,b,selected);*frequency=selected;return 1;
}
int sony_plus_time_extract(const float input[256],sony_plus_time_common *common,sony_plus_time_band *band,uint32_t selection_mode,int32_t peak,uint32_t maximum,uint32_t backend){
    if(!input||!common||!band||selection_mode>1||maximum>15||(maximum&&(band->first_note>48||maximum>48-band->first_note)))return -1;
    float work[256],power[129];for(uint32_t i=0;i<256;++i)work[i]=input[i];
    const float initial=spill(sony_plus_time_dot(work,work,256,backend));float previous=initial;int32_t keep=peak!=-1;uint32_t accepted=0;
    while(accepted<maximum){
        if(accepted){sony_plus_time_fft_power256(work,256,power,backend);peak=sony_plus_time_largest_positive(power,129);keep=peak!=-1;}
        if(!keep)break;float amplitude;int32_t phase,frequency;
        if(sony_plus_time_refine(work,peak,&amplitude,&phase,&frequency,backend)!=1){keep=0;break;}
        sony_plus_time_note *note=common->notes+band->first_note+accepted;
        note->amplitude=sony_plus_time_amplitude_code(amplitude);note->phase=sony_plus_time_phase_code(phase);note->frequency=(uint32_t)frequency;
        amplitude=time_amplitude_levels[note->amplitude];phase=(int32_t)sony_plus_time_expand_phase(note->phase);
        sony_plus_time_remove_tone(work,work,amplitude,phase,frequency,0,256,backend);
        sony_plus_time_continue(work,0,256,initial,&previous,&keep,backend);
        if(!keep)break;++accepted;
    }
    band->count=accepted;return sony_plus_time_filter_notes(common,band,selection_mode);
}
int sony_plus_time_synthesize(const sony_plus_time_common *common,const sony_plus_time_band *band,float *output,int32_t start,uint32_t count,uint32_t invert,uint32_t channel,uint32_t backend){
    if(!common||!band||!output||count>128||count%4||band->count>15||(band->count&&(band->first_note>48||band->count>48-band->first_note)))return -1;
    for(uint32_t i=0;i<count;++i)output[i]=0;
    for(uint32_t n=0;n<band->count;++n){const sony_plus_time_note *note=common->notes+band->first_note+n;if(note->amplitude>63)return -1;
        const float amp=time_amplitude_levels[note->amplitude];uint32_t phase=sony_plus_time_expand_phase(note->phase)+((uint32_t)start-128u)*note->frequency;
        for(uint32_t i=0;i<count;++i){double value=(double)amp*time_sine[phase&2047];if(backend)value=spill(value);output[i]=spill((double)output[i]+value);phase+=note->frequency;}
    }
    if(channel==1&&invert)for(uint32_t i=0;i<count;++i)output[i]=spill(-(double)output[i]);return 0;
}
int sony_plus_time_overlap(const sony_plus_time_common *current,const sony_plus_time_band *current_band,uint32_t current_invert,const sony_plus_time_common *previous,const sony_plus_time_band *previous_band,uint32_t previous_invert,float output[128],uint32_t channel,uint32_t backend){
    float a[128],b[128];if(sony_plus_time_synthesize(current,current_band,a,128,128,current_invert,channel,backend)||sony_plus_time_synthesize(previous,previous_band,b,0,128,previous_invert,channel,backend))return -1;
    for(uint32_t i=0;i<128;++i){if(current_band->count)a[i]=spill((double)a[i]*time_overlap_new[i]);if(previous_band->count)b[i]=spill((double)b[i]*time_overlap_old[i]);output[i]=spill((double)a[i]+b[i]);}return 0;
}
int sony_plus_time_filter_notes(sony_plus_time_common *common,sony_plus_time_band *band,uint32_t mode){
    if(!common||!band||mode>1||band->count>15||(band->count&&(band->first_note>48||band->count>48-band->first_note)))return -1;
    sony_plus_time_note saved[16];int32_t frequencies[16],order[16];uint32_t count=0;
    for(uint32_t i=0;i<band->count;++i){const sony_plus_time_note n=common->notes[band->first_note+i];if(mode||((n.fine_amplitude+1u)*n.amplitude)!=0)saved[count++]=n;}
    band->count=count;for(uint32_t i=0;i<count;++i)frequencies[i]=(int32_t)saved[i].frequency;sony_plus_time_sort_frequency(frequencies,order,count);
    for(uint32_t i=0;i<count;++i)common->notes[band->first_note+i]=saved[(uint32_t)order[i]];return 0;
}
int sony_plus_time_continue(const float *input,uint32_t start,uint32_t count,float initial_energy,float *previous_energy,int32_t *keep,uint32_t backend){
    if(!input||!previous_energy||!keep||start+count>256)return 0;
    const double energy=sony_plus_time_dot(input+start,input+start,count,backend);if(energy>=*previous_energy)*keep=0;*previous_energy=spill(energy);
    return energy/(double)initial_energy<0.10000000149011612;
}
