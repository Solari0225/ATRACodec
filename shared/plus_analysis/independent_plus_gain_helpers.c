#include "independent_plus_gain_helpers.h"
#include <string.h>
static float absolute(float value){uint32_t word;memcpy(&word,&value,4);word&=0x7fffffffu;memcpy(&value,&word,4);return value;}
float sony_recovered_plus_gain_maximum(const float *values,int32_t count){
    float result=values[0];
    for(int32_t i=1;i<count;++i)if(result<values[i])result=values[i];
    return result;
}
float sony_recovered_plus_gain_absmaximum(const float *values,int32_t count){
    float result=absolute(values[0]);
    for(int32_t i=1;i<count;++i){const float v=absolute(values[i]);if(result<v)result=v;}
    return result;
}
int32_t sony_recovered_plus_gain_level_index(int32_t exponent){
    if(exponent < -6)return -1;
    return exponent>=9?15:exponent+6;
}
int32_t sony_recovered_plus_gain_above_unity(const sony_plus_gain_points *points){
    if(!points||points->count>7)return -1;
    int32_t maximum=6;
    for(uint32_t i=0;i<points->count;++i){
        if(points->level[i]>15)return -1;
        if(maximum<(int32_t)points->level[i])maximum=(int32_t)points->level[i];
    }
    return maximum-6;
}
int32_t sony_recovered_plus_gain_envelope_turns(float initial_floor,const float envelope[32]){
    float current=envelope[0]<initial_floor?initial_floor:envelope[0];
    int32_t count=0,last_direction=0,last_counted_direction=0,elapsed=3;
    for(uint32_t i=1;i<32;++i){
        const float value=envelope[i];
        if(current<value){
            const double threshold=(double)current*4.0;
            if(threshold<=(double)value){
                if(last_direction!=1&&last_counted_direction!=1&&elapsed>=3){
                    ++count;last_counted_direction=1;
                }
                last_direction=1;elapsed=0;
            }
            current=value;
        }else if((double)current>=(double)value*4.0){
            if(last_direction!=2){
                if(last_counted_direction!=2){++count;last_counted_direction=2;}
                last_direction=2;elapsed=0;
            }
            current=value;
        }
        ++elapsed;
    }
    return count;
}
double sony_recovered_plus_gain_floor_ratio(float denominator,float numerator){
    volatile double value=denominator>4.0f?(double)numerator/(double)denominator:
        (double)numerator*0.25;
    return value;
}
