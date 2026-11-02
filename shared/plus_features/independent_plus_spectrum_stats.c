#include "independent_plus_spectrum_stats.h"
typedef union spectrum_bits {float f;uint32_t u;} spectrum_bits;
static float fvalue(uint32_t bits){spectrum_bits a;a.u=bits;return a.f;}
static float fabsolute(float value){spectrum_bits a;a.f=value;a.u&=0x7fffffffu;return a.f;}
static int finite_value(float value){spectrum_bits a;a.f=value;return (a.u&0x7f800000u)!=0x7f800000u;}
static double scalar_absolute_sum(const float* x,uint32_t count){
    double sum=0.0;
    for(uint32_t i=0;i<count;i+=4){
        double term=(double)fabsolute(x[i])+(double)fabsolute(x[i+1]);
        term+=(double)fabsolute(x[i+3]);
        term+=(double)fabsolute(x[i+2]);
        sum+=term;
    }
    return sum;
}
static float vector_absolute_sum(const float* x,uint32_t count){
    float a=0.0f,b=0.0f,c=0.0f,d=0.0f;
    for(uint32_t i=0;i<count;i+=4){
        a+=fabsolute(x[i]);b+=fabsolute(x[i+1]);
        c+=fabsolute(x[i+2]);d+=fabsolute(x[i+3]);
    }
    const float lower=a+b,upper=c+d;
    return lower+upper;
}
int plus_spectrum_low_ratios(const float x[256],uint32_t backend,float ratios[2]){
    if(!x||!ratios)return -1;
    for(uint32_t i=0;i<256;++i)if(!finite_value(x[i]))return -1;
    const double low=(backend?(double)vector_absolute_sum(x,16):scalar_absolute_sum(x,16))*(double)fvalue(0x3d800000u);
    double mid=(backend?(double)vector_absolute_sum(x+16,112):scalar_absolute_sum(x+16,112))*(double)fvalue(0x3c1248d8u);
    if(backend)mid=(float)mid; /* Original SSE stores this denominator first. */
    const double high=(backend?(double)vector_absolute_sum(x+128,128):scalar_absolute_sum(x+128,128))*(double)fvalue(0x3c000000u);
    ratios[0]=mid>0.0?(float)(low/mid):1.0f;
    ratios[1]=high>0.0?(float)(low/high):1.0f;
    return 0;
}
int plus_spectrum_peakiness(const float x[2048],const float peak[32],
    uint32_t units,uint32_t backend,float ratio[32]){
    if(!x||!peak||!ratio||units>32)return -1;
    for(uint32_t i=0;i<plus_feature_quant_start(units);++i)if(!finite_value(x[i]))return -1;
    for(uint32_t q=0;q<units;++q)if(!finite_value(peak[q]))return -1;
    for(uint32_t q=0;q<units;++q){
        if(peak[q]>0.0f){
            const uint32_t first=plus_feature_quant_start(q),n=plus_feature_quant_count(q),quarter=n/4;
            double sum=0.0;
            if(backend)sum=(double)vector_absolute_sum(x+first,n);
            else for(uint32_t i=0;i<quarter;++i){
                double term=(double)fabsolute(x[first+3*quarter+i])+(double)fabsolute(x[first+2*quarter+i]);
                term+=(double)fabsolute(x[first+quarter+i]);
                term+=(double)fabsolute(x[first+i]);
                sum+=term;
            }
            ratio[q]=(float)(((double)n*(double)peak[q])/sum);
        }else ratio[q]=1.0f;
    }
    for(uint32_t q=units;q<32;++q)ratio[q]=1.0f;
    return 0;
}
int32_t plus_spectrum_gain_spread(const plus_given_gain_band* band){
    if(!band||band->points>7)return -1;
    int32_t low=6,high=6;
    for(uint32_t i=0;i<band->points;++i){
        const int32_t level=(int32_t)band->levels[i];
        if(level<low)low=level;
        if(level>high)high=level;
    }
    return high-low>=4;
}
static int equal_gain(const plus_given_gain_band* a,const plus_given_gain_band* b){
    if(a->points!=b->points)return 0;
    for(uint32_t i=0;i<a->points;++i)
        if(a->levels[i]!=b->levels[i]||a->locations[i]!=b->locations[i])return 0;
    return 1;
}
int plus_spectrum_gain_header(plus_given_gain_channel* gain,
    const plus_given_gain_band previous[16],uint32_t bands,int16_t* activity){
    if(!gain||!previous||!activity||bands>16)return -1;
    for(uint32_t b=0;b<16;++b)if(gain->bands[b].points>7||previous[b].points>7)return -1;
    *activity=-1;
    for(uint32_t b=0;b<2;++b)if(plus_spectrum_gain_spread(gain->bands+b)){*activity=0;break;}
    if(*activity!=0)for(uint32_t b=0;b<2;++b)if(plus_spectrum_gain_spread(previous+b)){*activity=1;break;}
    uint32_t used=bands;
    while(used&&gain->bands[used-1].points==0)--used;
    gain->repeat_bands=used;gain->num_bands=used;gain->present=used?1u:0u;
    if(used>1){
        uint32_t last=used-1;
        while(last&&equal_gain(gain->bands+last,gain->bands+last-1))--last;
        gain->num_bands=last+1;
    }
    gain->repeat_flag=gain->repeat_bands!=gain->num_bands;
    return 0;
}
