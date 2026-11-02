#include "independent_plus_phase.h"
#include "independent_plus_math.h"
#include <string.h>
#include "phase_tables.inc"
static float spill(double x){volatile float result=(float)x;return result;}
static float number(uint32_t bits){float result;memcpy(&result,&bits,4);return result;}
static float absolute(float x){uint32_t word;memcpy(&word,&x,4);return number(word&0x7fffffffu);}
static float sum_abs(const float samples[128]){
    float lane[4]={0.,0.,0.,0.};
    for(uint32_t i=0;i<128;i+=4)for(uint32_t j=0;j<4;++j)lane[j]=spill((double)lane[j]+(double)absolute(samples[i+j]));
    const float a=spill((double)lane[0]+(double)lane[1]),b=spill((double)lane[2]+(double)lane[3]);return spill((double)a+(double)b);
}
int32_t sony_recovered_plus_coherence_boundary(int32_t profile_id,int32_t sample_rate){
    if(profile_id<0||profile_id>31)return -1;
    return sample_rate==44100?coherence_boundary_44100_i32_32[profile_id]:coherence_boundary_other_i32_32[profile_id];
}
int sony_recovered_plus_phase_mix(float left[16][128],float right[16][128],
    float coefficient[16],int32_t *coherence_boundary,int32_t profile_id,
    int32_t sample_rate,uint32_t bands)
{
    if(!left||!right||!coefficient||!coherence_boundary||profile_id<0||profile_id>31||bands>16)return -1;
    *coherence_boundary=sony_recovered_plus_coherence_boundary(profile_id,sample_rate);
    if(profile_id>=19)return 0;
    const uint32_t *limit=profile_id>=15?stereo_phase_limit_high_f32_16:profile_id>=13?stereo_phase_limit_mid_f32_16:stereo_phase_limit_low_f32_16;
    for(uint32_t band=0;band<bands;++band){
        float difference[128];for(uint32_t i=0;i<128;++i)difference[i]=spill((double)left[band][i]-(double)right[band][i]);
        const float ls=sum_abs(left[band]),rs=sum_abs(right[band]),ds=sum_abs(difference);
        const float ratio=(ls==0.f&&rs==0.f)?0.f:spill((double)ds/((double)ls+(double)rs));
        double target=0.;
        if(!(ds==0.f&&ls==0.f&&rs==0.f)&&ratio<=0x1.fffcp-1f){
            const double maximum=(double)number(limit[band]);
            if(ratio<0x1p-15f)target=maximum*.5;
            else{
                const double base=maximum*.25;
                const double angle=sony_plus_math_atan((double)ratio*10.-5.);
                target=base-(base*(double)number(0x3f3a660cu))*angle;
                const float cap=spill(maximum*.5);if(target>(double)cap)target=cap;if(target<0.)target=0.;
            }
        }
        const float delta=spill((target-(double)coefficient[band])*(1./64.));double ramp=coefficient[band];
        for(uint32_t i=0;i<64;++i){
            const float a=spill(ramp),b=spill(1.-(double)a),l=left[band][i],r=right[band][i];
            const float la=spill((double)l*(double)a),ra=spill((double)r*(double)a),lb=spill((double)l*(double)b),rb=spill((double)r*(double)b);
            left[band][i]=spill((double)lb+(double)ra);right[band][i]=spill((double)rb+(double)la);ramp+=(double)delta;
        }
        const float a=spill(target),b=spill(1.-(double)a);
        for(uint32_t i=64;i<128;++i){
            const float l=left[band][i],r=right[band][i],la=spill((double)l*(double)a),ra=spill((double)r*(double)a),lb=spill((double)l*(double)b),rb=spill((double)r*(double)b);
            left[band][i]=spill((double)lb+(double)ra);right[band][i]=spill((double)rb+(double)la);
        }
        coefficient[band]=a;
    }
    return 0;
}
