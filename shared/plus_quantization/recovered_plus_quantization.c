#include "recovered_plus_quantization.h"
#include "recovered_plus_cost.h"
#include "../plus_fields/independent_plus_spectrum.h"
#include "quantization_math.inc"
typedef union plus_float_word {float f;uint32_t u;} plus_float_word;
static float from_word(uint32_t u){plus_float_word v;v.u=u;return v.f;}
static uint32_t to_word(float f){plus_float_word v;v.f=f;return v.u;}
static const uint8_t short_map[60]={22,23,25,43,0,5,4,1,0,1,2,3,3,0,4,2,4,0,1,2,1,0,4,3,3,0,2,1,0,3,1,2,4,0,1,2,0,3,2,1,0,1,2,3,0,1,2,4,0,1,2,3,1,4,2,0,0,1,2,3};
uint32_t sony_recovered_plus_short_book(uint32_t bank,uint32_t wl,uint32_t candidate){
    return bank<=1&&wl>=1&&wl<=7&&candidate<4?7u*(short_map[4u*(7u*bank+wl)+candidate]+8u*bank)+wl:0;
}
int32_t sony_recovered_plus_quantize(const float *input,int16_t *output,
    uint32_t wl,uint32_t threshold_index,float factor,uint32_t n,uint32_t simd){
    if(!input||!output||wl<1||wl>7||threshold_index>15||n>128||n%4||
       factor<0.0f||(to_word(factor)&0x7f800000u)==0x7f800000u)return -1;
    for(uint32_t i=0;i<n;++i){uint32_t magnitude=to_word(input[i])&0x7fffffffu;if(magnitude>=0x7f800000u)return -1;}
    const float scale=from_word(plus_quant_scale_bits[wl]);
    volatile float threshold=(float)((double)factor*(double)from_word(plus_quant_threshold_bits[16u*wl+threshold_index]));
    const double magic=(double)from_word(0x4b400000u);
    for(uint32_t i=0;i<n;++i){
        const float value=input[i],magnitude=from_word(to_word(value)&0x7fffffffu);
        if(simd){
            volatile float product=value*scale;
            if(!(threshold<=magnitude))product=0.0f;
            if(product>=2147483648.0f||product<=-32768.0f)output[i]=-32768;
            else if(product>=32767.0f)output[i]=32767;
            else {volatile float rounded=(float)((double)product+magic);output[i]=(int16_t)(uint16_t)to_word(rounded);}
        }else{
            volatile float difference=(float)((double)threshold-(double)magnitude);
            volatile double product=(double)value*(double)scale;
            volatile float rounded=(float)(product+magic);
            output[i]=(int16_t)(uint16_t)((to_word(difference)&0x80000000u)?to_word(rounded):0u);
        }
    }
    return 0;
}
int32_t sony_recovered_plus_abs_count(const int16_t *input,int16_t *output,uint32_t n){
    if(!input||!output||n>128||n%4)return -1;
    uint32_t count=0;
    for(uint32_t i=0;i<n;++i){const int32_t v=input[i];output[i]=(int16_t)(uint16_t)(v<0?-v:v);count+=v!=0;}
    return (int32_t)count;
}
int32_t sony_recovered_plus_pack(const int16_t *input,uint32_t n,uint32_t group,
    uint32_t bits,uint32_t mask,int16_t *output,uint32_t simd){
    if(!input||!output||n>128||n%16||(group!=1&&group!=2&&group!=4)||bits<1||bits>6||
        (group>1&&bits*group>8)||mask>65535)return -1;
    if(group>1||simd)mask=(1u<<bits)-1u;
    for(uint32_t i=0;i<n/group;++i){uint32_t packed=0;
        for(uint32_t j=0;j<group;++j)packed=(packed<<bits)|((uint16_t)input[i*group+j]&mask);
        output[i]=(int16_t)(uint16_t)packed;
    }
    return 0;
}
int32_t sony_recovered_plus_quantization_cost(uint32_t bank,uint32_t wl,uint32_t threshold_index,
    float factor,uint32_t n,const float *input,uint32_t simd,int16_t costs[4]){
    if(!costs||bank>1||(n!=0&&n!=16&&n!=32&&n!=64&&n!=128))return -1;
    int16_t quantized[128],symbols[128];uint32_t sign_counts[128],sign_values[128];
    if(sony_recovered_plus_quantize(input,quantized,wl,threshold_index,factor,n,simd))return -1;
    uint32_t nonzero=0;for(uint32_t i=0;i<n;++i)nonzero+=quantized[i]!=0;
    for(uint32_t candidate=0;candidate<4;++candidate){
        const uint32_t index=sony_recovered_plus_short_book(bank,wl,candidate);
        const plus_spectrum_descriptor *d=plus_spectrum_book(index);if(!d)return -1;
        if(plus_spectrum_make_symbols(quantized,n,index,symbols,sign_counts,sign_values)<0)return -1;
        const int32_t cost=sony_recovered_plus_packed_cost(index,symbols,n);
        if(cost<0)return -1;costs[candidate]=(int16_t)(uint16_t)(cost+(d->extra_signs?nonzero:0));
    }
    return 0;
}
