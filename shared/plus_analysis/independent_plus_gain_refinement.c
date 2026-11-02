#include "independent_plus_gain_refinement.h"
#include <string.h>
static float spill(double value){volatile float result=(float)value;return result;}
static int32_t transitions(const int32_t levels[33]){
    int32_t count=0;for(uint32_t i=0;i<32;++i)if(levels[i]!=levels[i+1])++count;return count;
}
int32_t sony_recovered_plus_normalize_gain_levels(int32_t levels[33]){
    if(!levels)return -1;
    const int32_t last=levels[32];
    if(last)for(uint32_t i=0;i<33;++i)levels[i]-=last;
    for(uint32_t i=0;i<33;++i){if(levels[i]<-6)levels[i]=-6;else if(levels[i]>9)levels[i]=9;}
    return transitions(levels);
}
int32_t sony_recovered_plus_prune_gain_candidates(int32_t levels[33],
    sony_plus_gain_candidate *candidates,uint32_t candidate_count,int32_t differences[32]){
    if(!levels||!differences||candidate_count>32||(candidate_count&&!candidates))return -1;
    int32_t count=sony_recovered_plus_normalize_gain_levels(levels);
    if(count<=7||!candidate_count)return count;
    float score[32];uint32_t order[32];
    for(uint32_t i=0;i<candidate_count;++i){
        score[i]=spill((double)candidates[i].priority+(double)candidates[i].score);
        if(!(score[i]<65536.0f))return -1;
        order[i]=i;uint32_t j=i;
        while(j&&score[order[j]]<score[order[j-1]]){const uint32_t t=order[j];order[j]=order[j-1];order[j-1]=t;--j;}
    }
    for(uint32_t i=0;count>7;++i){
        if(i==candidate_count)return -1;
        sony_plus_gain_candidate *candidate=candidates+order[i];
        const int32_t start=candidate->start,end=candidate->end>33?33:candidate->end;
        if(start<0||start>32||end<1||end<start)return -1;
        for(int32_t j=start;j<end;++j)levels[j]-=candidate->exponent_delta;
        if(start>0)differences[start-1]+=candidate->exponent_delta;
        if(end<33)differences[end-1]-=candidate->exponent_delta;
        candidate->bound=65536.0f;count=transitions(levels);
    }
    return sony_recovered_plus_normalize_gain_levels(levels);
}
int sony_recovered_plus_refine_gain_baseline(sony_plus_gain_baseline *record,
    const sony_plus_gain_candidate *candidates,uint32_t candidate_count,int32_t minimum_index){
    if(!record||candidate_count>32||(candidate_count&&!candidates))return -1;
    float baseline=record->selected;
    for(uint32_t i=0;i<candidate_count;++i){
        const sony_plus_gain_candidate *candidate=candidates+i;
        if(candidate->end>32&&candidate->start-1>=minimum_index&&baseline>candidate->bound){
            baseline=candidate->bound;minimum_index=candidate->start-1;record->state=1;
        }
    }
    record->selected=baseline;return 0;
}
static float power(int32_t exponent){uint32_t word=(uint32_t)(127+exponent)<<23;float value;memcpy(&value,&word,4);return value;}
int sony_recovered_plus_weighted_gain_maximum(const float envelope[32],
    const int32_t levels[32],float *maximum){
    if(!envelope||!levels||!maximum)return -1;
    for(uint32_t i=0;i<32;++i)if(levels[i]<-6||levels[i]>9)return -1;
    float scale=power(levels[0]);*maximum=spill((double)scale*(double)envelope[0]);
    for(uint32_t i=1;i<32;++i){
        if(levels[i]!=levels[i-1])scale=power(levels[i]);
        const double value=(double)scale*(double)envelope[i];
        if(value>(double)*maximum)*maximum=spill(value);
    }
    return 0;
}
