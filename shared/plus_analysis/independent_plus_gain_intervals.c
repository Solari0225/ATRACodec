#include "independent_plus_gain_intervals.h"
#include "independent_plus_gain_log.h"
#include <string.h>
#include "gain_interval_tables.inc"
static float spill(double value){volatile float result=(float)value;return result;}
/* Exact native3f1510integer-shift semantics, including its signed32bit base. */
static double power(int32_t exponent){
    const uint32_t shift=(exponent<0?0u-(uint32_t)exponent:(uint32_t)exponent)&31u;
    const uint32_t word=1u<<shift;int32_t base;memcpy(&base,&word,4);
    return exponent<0?1.0/(double)base:(double)base;
}
int sony_recovered_plus_insert_gain_interval(const sony_plus_gain_segment *segment,
    const sony_plus_gain_interval *interval,sony_plus_gain_interval_queue *queue,
    const float envelope[65],const sony_plus_gain_refinement_record *record,
    sony_plus_gain_candidate_state *state,float reference_maximum){
    if(!segment||!interval||!queue||!envelope||!record||!state||state->candidate_count>32||queue->count<0||queue->count>32||
        interval->start<1||interval->start>32||interval->end<interval->start||interval->end>64||
        !(interval->maximum>0.0f)||!(record->low_band_scale>0.0f))return -1;
    const int32_t start=interval->start,end=interval->end;
    const float left_numerator=spill(power(state->levels[start-1])*(double)envelope[start-1]);
    const float left=spill((double)left_numerator/(power(state->levels[start])*(double)interval->maximum));
    double right;
    if(end<33){
        const float numerator=spill(power(state->levels[end])*(double)envelope[end]);
        right=(double)numerator/(power(state->levels[end-1])*(double)interval->maximum);
    }else right=(double)envelope[end]/(double)interval->maximum;
    const double lesser=(double)left<right?(double)left:right;
    const float ratio=spill(lesser/(double)record->low_band_scale);
    if(!(ratio>0.0f))return -1;
    int32_t delta=(int32_t)sony_recovered_plus_gain_log_measure((double)ratio);
    if(!segment->classification&&(double)interval->maximum*2.0>(double)reference_maximum)delta=0;
    else if(delta==1&&interval->maximum<5.0f&&record->maximum>100.0f)delta=0;
    if(delta>0){
        int32_t count=state->transitions+(state->differences[start-1]==0);
        if(end<33&&state->differences[end-1]==0)++count;
        if(count<=7){
            if(state->candidate_count==32)return -1;
            sony_plus_gain_candidate *candidate=state->candidate+state->candidate_count++;
            candidate->score=ratio;candidate->start=start;candidate->end=end;
            candidate->priority=end-start;candidate->exponent_delta=delta;candidate->bound=interval->maximum;
            const int32_t stop=end<33?end:33;
            for(int32_t i=start;i<stop;++i)state->levels[i]+=delta;
            state->differences[start-1]=state->levels[start-1]-state->levels[start];
            if(end<33)state->differences[end-1]=state->levels[end-1]-state->levels[end];
            state->transitions=count;
        }
    }
    if(state->transitions<7&&start<32&&end-start>7){
        if(queue->count==32)return -1;
        const uint32_t i=(uint32_t)queue->count++;
        queue->start[i]=start;queue->end[i]=end;queue->maximum[i]=interval->maximum;
        queue->classification[i]=delta>0?9:segment->classification;
    }
    return 0;
}
int sony_recovered_plus_refine_gain_intervals(const sony_plus_gain_refinement_record *record,
    sony_plus_gain_candidate_state *state,const sony_plus_gain_interval_queue *queue,
    const float envelope[65],float reference_maximum){
    if(!record||!state||!queue||!envelope||queue->count<0||queue->count>32)return -1;
    const float threshold=spill((double)record->low_band_scale*2.0);
    for(int32_t item=0;item<queue->count;++item){
        int32_t start=queue->start[item],end=queue->end[item],classification=queue->classification[item];
        float maximum=queue->maximum[item];
        int32_t attempts=7-state->transitions;if(attempts>3)attempts=3;
        while(attempts>0&&state->transitions<7&&start<=32){
            if(start<0||end<start||end>64||classification<0||classification>10)return -1;
            sony_plus_gain_interval_queue pending={0};
            sony_plus_gain_segment segment={0};segment.classification=classification;
            sony_plus_gain_interval interval={start,start,4.0f};
            int32_t run_start=start,run_end=start;
            for(int32_t j=start;j<end;++j){
                if((double)threshold*(double)envelope[j]>(double)maximum){
                    if(run_end>32)break;
                    if(run_start>0&&run_start<=32&&run_end-run_start>gain_interval_minimum_length[classification]){
                        interval.start=run_start;interval.end=run_end;
                        if(sony_recovered_plus_insert_gain_interval(&segment,&interval,&pending,envelope,record,state,reference_maximum))return -1;
                    }
                    run_start=run_end=j+1;interval.maximum=4.0f;
                    if(state->transitions>=7)break;
                }else{
                    ++run_end;if(interval.maximum<envelope[j])interval.maximum=envelope[j];
                }
            }
            if(state->transitions<7&&run_start>0&&run_start<=32&&
                    run_end-run_start>gain_interval_minimum_length[classification]&&
                    (start!=run_start||end!=run_end)){
                interval.start=run_start;interval.end=run_end;
                if(sony_recovered_plus_insert_gain_interval(&segment,&interval,&pending,envelope,record,state,reference_maximum))return -1;
            }
            if(pending.count<1)break;
            int32_t selected=0,length=pending.end[0]-pending.start[0];
            for(int32_t i=1;i<pending.count;++i)if(length<pending.end[i]-pending.start[i]){selected=i;length=pending.end[i]-pending.start[i];}
            start=pending.start[selected];end=pending.end[selected];maximum=pending.maximum[selected];classification=pending.classification[selected];
            --attempts;
        }
    }
    return 0;
}
