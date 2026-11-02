#include "recovered_plus_time_math.h"
/* Original numerical domains captured by prepare_capture.py. */
#include "time_math.inc"
static float spill(double x){volatile float value=(float)x;return value;}
void sony_plus_time_clear(sony_plus_time_common *common,sony_plus_time_band *bands,uint32_t count){
    if(!common||!bands||count>16)return;common->present=0;common->bands=1;
    for(uint32_t b=0;b<count;++b){sony_plus_time_band *v=bands+b;
        v->envelope[0]=v->envelope[1]=v->envelope[4]=v->envelope[5]=v->envelope[6]=0;v->envelope[7]=32;v->count=0;}
}
int sony_plus_time_mix(const float *left,const float *right,float *output,
    uint32_t count,uint32_t op,uint32_t backend){
    if(!left||!right||!output||count>256||op>2)return -1;
    const uint32_t n=backend?((count+3)&~3u):(count&~3u);
    for(uint32_t i=0;i<n;++i){double value=op==1?(double)left[i]+right[i]:(double)left[i]-right[i];
        if(op){if(backend)value=spill(value);value*=.5;}output[i]=spill(value);}
    return 0;
}
uint32_t sony_plus_time_amplitude_code(float amplitude){
    const float value=spill((double)amplitude*time_amplitude_input_scale);
    if(time_amplitude_levels[0]>value)return 0;
    int32_t at=32,step=16;
    do{at+=time_amplitude_levels[at]>value?-step:step;step/=2;}while(step>0);
    if(at<63&&time_amplitude_levels[at]<value)++at;
    return (uint32_t)at;
}
uint32_t sony_plus_time_phase_code(int32_t phase){
    /* Captured multiplier1/64 and offset1/2 make the complete finite I32
     * phase domain exactly expressible without a host CRT/floating cast. */
    const int64_t n=(int64_t)phase+32;
    const int64_t rounded=n>=0?n/64:-((-n+63)/64);
    return (uint32_t)rounded&31;
}
uint32_t sony_plus_time_expand_phase(uint32_t phase){return (phase&31u)<<6;}
int sony_plus_time_remove_tone(const float input[256],float output[256],
    float amplitude,int32_t phase,int32_t frequency,uint32_t start,
    uint32_t end,uint32_t backend){
    if(!input||!output||start>end||end>256||start%4||end%4)return -1;
    uint32_t angle=(uint32_t)phase+((uint32_t)start-128u)*(uint32_t)frequency;
    for(uint32_t i=start;i<end;++i){
        double value=(double)amplitude*time_sine[angle&2047u];
        if(backend)value=spill(value);
        output[i]=spill((double)input[i]-value);angle+=(uint32_t)frequency;
    }return 0;
}
int sony_plus_time_sort_power(float *v,int32_t *indices,uint32_t count){
    if(!v||!indices||count>256)return -1;
    for(uint32_t i=0;i<count;++i)indices[i]=(int32_t)i;
    uint32_t gap=1;while(gap<=count)gap=gap*3+1;
    for(gap/=3;gap;gap/=3)for(uint32_t i=gap;i<count;++i){
        const float value=v[i];uint32_t at=i;
        while(at>=gap&&value>v[at-gap]){v[at]=v[at-gap];
            const int32_t old=indices[at];indices[at]=indices[at-gap];indices[at-gap]=old;at-=gap;}
        v[at]=value;
    }return 0;
}
int sony_plus_time_sort_frequency(int32_t *v,int32_t *indices,uint32_t count){
    if(!v||!indices||count>16)return -1;
    for(uint32_t i=0;i<count;++i){int32_t lowest=v[0];uint32_t at=0;
        for(uint32_t j=1;j<count;++j)if(v[j]<lowest){lowest=v[j];at=j;}
        indices[i]=(int32_t)at;v[at]=0x8000;
    }return 0;
}
int32_t sony_plus_time_largest_positive(const float *v,uint32_t count){
    if(!v||count>256)return -1;float largest=0;int32_t at=-1;
    for(uint32_t i=0;i<count;++i)if(v[i]>largest){largest=v[i];at=(int32_t)i;}
    return at;
}
double sony_plus_time_peak_ratio(const float *v,uint32_t count){
    if(!v||!count||count>256)return 0;float largest=v[0];double sum=v[0];
    for(uint32_t i=1;i<count;++i){if(v[i]>largest)largest=v[i];sum+=(double)v[i];}
    return sum!=0?((double)largest/sum)*(double)count:0;
}
double sony_plus_time_dot(const float *left,const float *right,uint32_t count,uint32_t backend){
    if(!left||!right||count>256)return 0;const uint32_t n=(count+3)&~3u;
    if(backend){float lane[4]={0,0,0,0};
        for(uint32_t i=0;i<n;i+=4)for(uint32_t j=0;j<4;++j){const float product=spill((double)left[i+j]*right[i+j]);lane[j]=spill((double)lane[j]+product);}
        const float a=spill((double)lane[0]+lane[1]),b=spill((double)lane[2]+lane[3]);return spill((double)a+b);
    }
    double lane[4]={0,0,0,0};for(uint32_t i=0;i<n;i+=4)for(uint32_t j=0;j<4;++j)lane[j]+=(double)left[i+j]*right[i+j];
    return lane[0]+((lane[3]+lane[2])+lane[1]);
}
