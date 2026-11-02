#include "independent_plus_gain_math.h"
#include "independent_plus_mdct.h"
#include <string.h>
#include "gain_tables.inc"
static float number(uint32_t bits){float f;memcpy(&f,&bits,4);return f;}
static float power(int32_t exponent){return number((uint32_t)(127+exponent)<<23);}
static float product(float a,float b){volatile double v=(double)a*(double)b;return (float)v;}
static float transition(float coefficient,float sample,float scale){
    volatile double a=(double)coefficient*(double)sample;
    volatile double b=a*(double)scale;return (float)b;
}
static int valid(const sony_plus_gain_points *points){
    if(!points||points->count>7)return 0;
    for(uint32_t i=0;i<points->count;++i)
        if(points->location[i]>31||points->level[i]>15||
           (i&&points->location[i]<=points->location[i-1]))return 0;
    return 1;
}
int sony_recovered_plus_apply_gain(float samples[128],const sony_plus_gain_points *points)
{
    if(!samples||!valid(points))return -1;
    if(!points->count)return 0;
    uint32_t position=0;
    int32_t current=gain_exponents[points->level[0]];
    for(uint32_t point=0;point<points->count;++point){
        const float scale=power(current);
        const uint32_t endpoint=4*points->location[point];
        for(;position<endpoint;++position)samples[position]=product(scale,samples[position]);
        const int32_t next=point+1<points->count?gain_exponents[points->level[point+1]]:0;
        samples[position]=product(scale,samples[position]);++position;
        if(current==next){
            for(uint32_t i=0;i<3;++i,++position)samples[position]=product(scale,samples[position]);
        }else if(current<next){
            const uint32_t index=(uint32_t)(next-current-1)*3;
            const float next_scale=power(next);
            for(uint32_t i=0;i<3;++i,++position)
                samples[position]=transition(number(gain_transition[index+i]),samples[position],next_scale);
        }else{
            const uint32_t index=(uint32_t)(current-next-1)*3;
            for(uint32_t i=0;i<3;++i,++position)
                samples[position]=transition(number(gain_transition[index+2-i]),samples[position],scale);
        }
        current=next;
    }
    return 0;
}
int sony_recovered_plus_gain_mdct(float state[192],float output[128],
    const sony_plus_gain_points *points,uint32_t reverse_flag)
{
    if(!state||!output||!valid(points))return -1;
    const int result=sony_recovered_plus_apply_gain(state+64,points);
    if(result)return result;
    const float scale=points->count?power(gain_exponents[points->level[0]]):1.0f;
    return sony_recovered_plus_mdct(state,output,points->window_flag,scale,reverse_flag);
}
