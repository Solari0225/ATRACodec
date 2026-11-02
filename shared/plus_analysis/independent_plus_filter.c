#include "independent_plus_filter.h"
#include <stdint.h>
#include <string.h>
#include "filter_tables.inc"
static float f32_add(float a,float b){volatile float v=a+b;return v;}
static float f32_sub(float a,float b){volatile float v=a-b;return v;}
static float f32_mul(float a,float b){volatile float v=a*b;return v;}
static float number(uint32_t word){float v;memcpy(&v,&word,4);return v;}
int sony_recovered_plus_filter(const float input[2048],float history[368],
    float output[16][128])
{
    if(!input||!history||!output)return -1;
    float samples[2416];
    memcpy(samples,history,368*sizeof(float));
    memcpy(samples+368,input,2048*sizeof(float));
    memcpy(history,input+1680,368*sizeof(float));
    for(uint32_t time=0;time<128;++time){
        float phase[32],folded[16];
        for(uint32_t i=0;i<32;++i){
            const uint32_t column=i<8||i>=24?i:(i<16?i+8:i-8);
            float sum=f32_mul(samples[16*time+i],number(filter_coefficients[column]));
            for(uint32_t tap=1;tap<12;++tap)
                sum=f32_add(sum,f32_mul(samples[16*time+32*tap+i],
                    number(filter_coefficients[32*tap+column])));
            phase[i]=sum;
        }
        for(uint32_t lane=0;lane<4;++lane){
            folded[lane]=f32_add(phase[8+lane],phase[7-lane]);
            folded[4+lane]=f32_add(phase[12+lane],phase[3-lane]);
            folded[8+lane]=f32_add(phase[16+lane],phase[31-lane]);
            folded[12+lane]=f32_add(phase[20+lane],phase[27-lane]);
        }
        for(uint32_t band=0;band<16;++band)
            output[band][time]=folded[filter_permutation[band]];
    }
    /* Original staged16-point band butterfly, preserving every binary32
     * rounding point and reversed-half accumulation. No host trigonometry. */
    for(uint32_t width=1;width<=8;width*=2){
        for(uint32_t group=0;group<16;group+=2*width){
            const float scale=number(filter_cosine[width-1+group]);
            for(uint32_t time=0;time<128;++time){
                for(uint32_t i=0;i<width;++i){
                    const float a=output[group+i][time];
                    const float b=output[group+width+i][time];
                    output[group+i][time]=f32_add(a,b);
                    output[group+width+i][time]=f32_mul(f32_sub(a,b),scale);
                }
                for(uint32_t i=0;i<width;++i)
                    output[group+i][time]=f32_add(output[group+i][time],
                        output[group+2*width-1-i][time]);
            }
        }
    }
    return 0;
}
