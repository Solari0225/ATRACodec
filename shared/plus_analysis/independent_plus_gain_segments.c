#include "independent_plus_gain_segments.h"
#include <string.h>
#include "gain_segments_tables.inc"
static float spill(double value){volatile float result=(float)value;return result;}
static float number(uint32_t word){float result;memcpy(&result,&word,4);return result;}
int32_t sony_recovered_plus_gain_segments(const float envelope[32],
    const int32_t levels[33],sony_plus_gain_segment segments[33],int32_t differences[32]){
    if(!envelope||!levels||!segments||!differences)return -1;
    uint32_t current=0;segments[0].start=0;segments[0].end=64;
    segments[0].previous_direction=0;segments[0].next_direction=0;segments[0].maximum=4.0f;
    for(int32_t i=0;i<32;++i){
        if(segments[current].maximum<envelope[i])segments[current].maximum=envelope[i];
        if(levels[i]!=levels[i+1]){
            differences[i]=levels[i]-levels[i+1];
            segments[current].end=i+1;
            const int32_t direction=levels[i]>levels[i+1]?1:2;
            segments[current].next_direction=direction;
            ++current;segments[current].start=i+1;segments[current].end=64;
            segments[current].previous_direction=direction;segments[current].next_direction=0;
            segments[current].maximum=4.0f;
        }
    }
    return (int32_t)current;
}
int sony_recovered_plus_classify_gain_segments(const float envelope[33],
    sony_plus_gain_segment *segments,uint32_t segment_count,
    float lower_reference,float upper_reference){
    if(!envelope||!segments||segment_count>33)return -1;
    for(uint32_t i=0;i<segment_count;++i){
        sony_plus_gain_segment *s=segments+i;
        const int32_t a=s->previous_direction,b=s->next_direction;
        if(a==1&&(s->start<0||s->start>32))return -1;
        if(b==2&&(s->end<1||s->end>33))return -1;
        if(a==0){
            if(b==1){s->classification=1;s->baseline=lower_reference;}
            else if(b==2){s->classification=2;s->baseline=envelope[s->end-1];}
            else{s->classification=0;s->baseline=s->maximum;}
        }else if(a==1){
            if(b==1){s->classification=5;s->baseline=envelope[s->start];}
            else if(b==2){s->classification=6;s->baseline=envelope[s->start]<envelope[s->end-1]?envelope[s->start]:envelope[s->end-1];}
            else{s->classification=4;s->baseline=envelope[s->start];}
        }else if(a==2){
            if(b==1){s->classification=9;s->baseline=s->maximum;}
            else if(b==2){s->classification=10;s->baseline=envelope[s->end-1];}
            else{s->classification=8;s->baseline=upper_reference;}
        }
    }
    return 0;
}
static int32_t baseline(sony_plus_gain_baseline *record,const float envelope[33],
    const int32_t differences[32],uint32_t band,float fallback,int32_t mode,uint32_t clear_state){
    if(!record||!envelope||!differences||band>=16)return -1;
    const float factor=spill((double)number(gain_baseline_band_scale[band])*(double)record->low_band_scale);
    float maximum=envelope[32];int32_t last=0;
    for(int32_t i=31;i>=0;--i){
        if(differences[i]){last=i;break;}
        if(maximum<envelope[i])maximum=envelope[i];
    }
    if(mode<=0){
        record->selected=record->lower;
        if(record->upper<record->lower&&(double)record->lower<(double)factor*(double)record->upper)record->selected=record->upper;
    }else if(differences[last]>0){
        record->selected=maximum;
        if(last<16&&(double)maximum<=(double)factor*(double)record->upper)record->selected=record->upper;
    }else{
        record->selected=fallback;
        if(last<16){
            const double scaled=(double)factor*(double)record->upper;
            if(scaled<(double)fallback&&(double)maximum<scaled)record->selected=record->upper;
        }
    }
    if(clear_state)record->state=0;
    return last;
}
int32_t sony_recovered_plus_gain_baseline_normal(sony_plus_gain_baseline *record,
    const float envelope[33],const int32_t differences[32],uint32_t band,float fallback,int32_t mode){
    return baseline(record,envelope,differences,band,fallback,mode,1);
}
int32_t sony_recovered_plus_gain_baseline_difference(sony_plus_gain_baseline *record,
    const float envelope[33],const int32_t differences[32],uint32_t band,float fallback,int32_t mode){
    return baseline(record,envelope,differences,band,fallback,mode,0);
}
