#include "independent_plus_coarse.h"
#include "coarse_math.inc"
typedef union feature_float_bits {float f;uint32_t u;} feature_float_bits;
static float value(uint32_t bits){feature_float_bits v;v.u=bits;return v.f;}
static uint32_t bits(float f){feature_float_bits v;v.f=f;return v.u;}
static int finite(float f){return (bits(f)&0x7f800000u)!=0x7f800000u;}
static float absolute(float f){return value(bits(f)&0x7fffffffu);}
uint32_t plus_feature_quant_start(uint32_t unit){return unit<=32?feature_qu_starts[unit]:0;}
uint32_t plus_feature_quant_count(uint32_t unit){return unit<32?feature_qu_counts[unit]:0;}
float plus_feature_coarse_scale(uint32_t index){return index<64?value(feature_scale_bits[index]):0.0f;}
int32_t plus_feature_coarse_index(float peak){
    const float product=(float)((double)peak*(double)value(feature_peak_multiplier_bits));
    const uint32_t v=bits(product),mantissa=v&0x7fffffu;
    int32_t index=(int32_t)((v>>23)&255u)*3;
    if(mantissa>=0x642a00u)index-=0x16a;
    else if(mantissa>=0x351800u)index-=0x16b;
    else if(mantissa>=0x0fbc00u)index-=0x16c;
    else index-=0x16d;
    return index<0?0:index>63?63:index;
}
static void coarse_unchecked(const float* spectrum,uint32_t units,
    int32_t* coarse,float* peak,uint32_t backend){
    for(uint32_t q=0;q<units;++q){
        const uint32_t start=feature_qu_starts[q],n=feature_qu_counts[q];
        float maximum=absolute(spectrum[start]);
        for(uint32_t k=1;k<n;++k){
            const float a=absolute(spectrum[start+k]);
            if(a>maximum)maximum=a;
        }
        /* SSE interleaves groups of four QUs and uses MAXPS(x,-MINPS(x)).
         * Equal zeros select the second operand, retaining the negation of
         * the last input's zero sign. Remainder QUs use scalar FABS. */
        if(backend&&q<(units&~3u)&&maximum==0.0f)
            maximum=value(bits(spectrum[start+n-1])^0x80000000u);
        peak[q]=maximum;coarse[q]=plus_feature_coarse_index(maximum);
    }
}
int plus_feature_coarse(const float spectrum[2048],uint32_t units,
    int32_t coarse[32],float peak[32],uint32_t backend){
    if(!spectrum||!coarse||!peak||units>32)return -1;
    for(uint32_t k=0;k<feature_qu_starts[units];++k)if(!finite(spectrum[k]))return -1;
    coarse_unchecked(spectrum,units,coarse,peak,backend);return 0;
}
static int valid_scales(const int32_t* coarse,uint32_t units){
    if(!coarse||units>32)return 0;
    for(uint32_t q=0;q<units;++q)if(coarse[q]<0||coarse[q]>63)return 0;
    return 1;
}
int plus_feature_normalize(float spectrum[2048],const int32_t coarse[32],uint32_t units){
    if(!spectrum||!valid_scales(coarse,units))return -1;
    for(uint32_t q=0;q<units;++q){
        const float reciprocal=(float)(1.0/(double)plus_feature_coarse_scale((uint32_t)coarse[q]));
        for(uint32_t k=feature_qu_starts[q];k<feature_qu_starts[q+1];++k)
            spectrum[k]=(float)((double)reciprocal*(double)spectrum[k]);
    }
    return 0;
}
int plus_feature_clip(float spectrum[2048],const int32_t coarse[32],uint32_t units){
    if(!spectrum||!valid_scales(coarse,units))return -1;
    const float upper=value(feature_clip_positive_bits),lower=-upper;
    for(uint32_t q=0;q<units;++q)if(coarse[q]>=63){
        for(uint32_t k=feature_qu_starts[q];k<feature_qu_starts[q+1];++k){
            if(spectrum[k]>upper)spectrum[k]=upper;
            else if(spectrum[k]<lower)spectrum[k]=lower;
        }
    }
    return 0;
}
int plus_feature_prepare_normalized(plus_feature_channel c[2],plus_feature_shared* shared,
    uint32_t units,uint32_t channels,uint32_t backend){
    if(!c||!shared||units>32||channels<1||channels>2)return -1;
    for(uint32_t ch=0;ch<channels;++ch){
        if(!valid_scales(c[ch].coarse,units))return -1;
        for(uint32_t q=0;q<units;++q)if(!finite(c[ch].numerator[q]))return -1;
        for(uint32_t k=0;k<feature_qu_starts[units];++k)if(!finite(c[ch].spectrum[k]))return -1;
    }
    if(channels==2){
        float difference[2048];
        for(uint32_t k=0;k<feature_qu_starts[units];++k)
            difference[k]=(float)((double)c[0].spectrum[k]-(double)c[1].spectrum[k]);
        coarse_unchecked(difference,units,shared->difference_coarse,shared->difference_peak,backend);
    }
    for(uint32_t ch=0;ch<channels;++ch){
        plus_feature_normalize(c[ch].spectrum,c[ch].coarse,units);
        plus_feature_clip(c[ch].spectrum,c[ch].coarse,units);
        for(uint32_t q=0;q<units;++q)c[ch].factor[q]=
            (float)((double)c[ch].numerator[q]/(double)plus_feature_coarse_scale((uint32_t)c[ch].coarse[q]));
    }
    return 0;
}
int plus_feature_round_limits(const float demand[32],const int16_t limit[32],
    uint32_t units,int32_t word_length[32]){
    if(!demand||!limit||!word_length||units>32)return -1;
    for(uint32_t q=0;q<units;++q){
        const double a=(double)demand[q]+0.5;
        if(!finite(demand[q])||a>=2147483648.0||a< -2147483648.0)return -1;
    }
    for(uint32_t q=0;q<units;++q){
        const int32_t a=(int32_t)((double)demand[q]+0.5);
        word_length[q]=a>limit[q]?limit[q]:a<1?1:a;
    }
    return 0;
}
