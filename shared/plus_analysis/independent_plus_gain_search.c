#include "independent_plus_gain_search.h"
#include "independent_plus_gain_log.h"
static float spill(double value){volatile float result=(float)value;return result;}
static double ratio(float denominator,float numerator){return denominator>4.0f?(double)numerator/(double)denominator:(double)numerator*0.25;}
static int32_t amount(double measure,int32_t count,uint32_t difference_mode,
    const sony_plus_gain_search_points *points,float *fraction,uint32_t truncate){
    double bias=0.5;
    if(difference_mode&&count&&measure>=2.0)bias=points->fraction[count-1];
    const int32_t value=(int32_t)(measure+(truncate?0.0:bias));
    if(difference_mode)*fraction=spill(measure-(double)value);
    return value;
}
int32_t sony_recovered_plus_search_gain_forward(const sony_plus_gain_forward_request *request,
    sony_plus_gain_forward_state *state,const float envelope[33],sony_plus_gain_search_points *points){
    if(!request||!state||!envelope||!points||request->length>32||request->step<1||request->step>2||
        request->point_count<0||request->point_count>7||request->opposite_count<0||request->opposite_count>7)return -1;
    int32_t count=request->point_count;float floor=request->floor,secondary=request->secondary_floor;
    if(request->length<=0)return count;
    for(int32_t i=0;i<request->length;i+=request->step){
        if(count+request->opposite_count>=6)return count;
        if(floor<envelope[i])floor=envelope[i];
        if(!count&&secondary>=0.0f){
            if(secondary<envelope[i])secondary=envelope[i];
            if(i>25&&floor>secondary)floor=secondary;
        }
        const float next=envelope[i+request->step];
        if((double)next>(double)floor*(double)request->threshold){
            const double measure=sony_recovered_plus_gain_log_measure(ratio(floor,next));
            float fraction=0.0f;const uint32_t truncate=!request->difference_mode&&state->truncate_first!=0;
            int32_t delta=amount(measure,count,request->difference_mode!=0,points,&fraction,truncate);
            if(truncate)state->truncate_first=0;
            if(delta>0){
                if(state->used_delta+delta>state->limit)delta=state->limit-state->used_delta;
                if(delta>0){
                    state->used_delta+=delta;state->total_delta+=delta;
                    points->increment[count]=delta;points->location[count]=i+request->step-1;
                    if(request->difference_mode)points->fraction[count]=fraction;
                    ++count;
                }
            }
        }
    }
    return count;
}
int32_t sony_recovered_plus_search_gain_backward(const sony_plus_gain_backward_request *request,
    sony_plus_gain_backward_state *state,const float envelope[33],const float paired_envelope[33],
    sony_plus_gain_search_points *points){
    if(!request||!state||!envelope||!paired_envelope||!points||request->stop<0||
        (request->step!=1&&request->step!=2&&request->step!=4)||
        request->point_count<0||request->point_count>7||request->opposite_count<0||request->opposite_count>7)return -1;
    int32_t count=request->point_count;float floor=request->floor;
    if(request->stop>=32)return count;
    for(int32_t i=32;i>request->stop;i-=request->step){
        if(floor<envelope[i]&&(i!=32||!request->freeze_lookahead))floor=envelope[i];
        const float previous=envelope[i-1];
        if((double)previous>(double)floor*(double)request->threshold){
            const double measure=sony_recovered_plus_gain_log_measure(ratio(floor,previous));
            float fraction=0.0f;int32_t delta=amount(measure,count,request->difference_mode!=0,points,&fraction,0);
            if(delta>0){
                if(state->used_delta+delta>state->limit)delta=state->limit-state->used_delta;
                if(delta>0){
                    if(count>=7)return -1;
                    state->used_delta+=delta;points->increment[count]=delta;points->location[count]=i;
                    if(request->difference_mode)points->fraction[count]=fraction;
                    if(!count)state->first_peak=floor;
                    for(int32_t j=i-1;j>i-request->step;--j){
                        if(paired_envelope[j]>envelope[i])break;
                        points->location[count]=j;
                    }
                    ++count;
                }
            }
            if(count+request->opposite_count>=7)return count;
        }
    }
    return count;
}
