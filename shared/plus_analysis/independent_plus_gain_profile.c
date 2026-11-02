#include "independent_plus_gain_profile.h"
#include <string.h>
#include "gain_interval_tables.inc"
#include "gain_profile_tables.inc"
static float spill(double value){volatile float result=(float)value;return result;}
static float number(uint32_t word){float result;memcpy(&result,&word,4);return result;}
static int enqueue(sony_plus_gain_interval_queue *queue,int32_t start,int32_t end,float maximum,int32_t classification){
    if(queue->count>=32)return -1;const uint32_t i=(uint32_t)queue->count++;
    queue->start[i]=start;queue->end[i]=end;queue->maximum[i]=maximum;queue->classification[i]=classification;return 0;
}
int sony_recovered_plus_refine_gain_profile(sony_plus_gain_refinement_record *record,
    const float envelope[65],int32_t levels[33],const sony_plus_gain_profile_request *request){
    if(!record||!envelope||!levels||!request||request->band>=16)return -1;
    sony_plus_gain_candidate_state state={0};sony_plus_gain_segment segments[33]={0};
    sony_plus_gain_interval_queue pending={0};memcpy(state.levels,levels,33*sizeof(int32_t));
    const int32_t original_transitions=sony_recovered_plus_gain_segments(envelope,levels,segments,state.differences);
    if(original_transitions<0||original_transitions>7)return -1;
    state.transitions=original_transitions;
    sony_plus_gain_baseline baseline={record->maximum,record->upper_maximum,record->low_band_scale,record->selected_baseline,record->baseline_state};
    const int32_t last=sony_recovered_plus_gain_baseline_normal(&baseline,envelope,state.differences,request->band,request->fallback,original_transitions);
    if(last<0)return -1;
    record->selected_baseline=baseline.selected;record->baseline_state=baseline.state;
    if(!request->difference_mode&&request->search_count>=2&&original_transitions<7){
        if(sony_recovered_plus_classify_gain_segments(envelope,segments,(uint32_t)original_transitions+1,request->lower_reference,request->upper_reference))return -1;
        double sum=0.0;for(uint32_t i=0;i<32;++i)sum+=(double)envelope[i];
        const float reference=spill(sum*(double)number(gain_profile_reference_factor));
        for(int32_t index=0;index<=original_transitions;++index){
            if(state.transitions>=7)continue;
            const sony_plus_gain_segment *segment=segments+index;
            const int32_t classification=segment->classification;
            if(classification<0||classification>10)return -1;
            const int32_t minimum=gain_interval_minimum_length[classification];
            if(segment->end-segment->start<=minimum)continue;
            const float factor=segment->next_direction?number(gain_profile_bounded_segment_factor):2.0f;
            const float threshold=spill((double)record->low_band_scale*(double)factor);
            int32_t run_start=segment->start,run_end=segment->start;
            sony_plus_gain_interval interval={run_start,run_end,4.0f};
            for(int32_t j=segment->start;j<segment->end;++j){
                if((double)threshold*(double)envelope[j]>(double)segment->baseline){
                    if(run_end>32)break;
                    if(run_end-run_start>minimum&&run_start>0){
                        interval.start=run_start;interval.end=run_end;
                        if(sony_recovered_plus_insert_gain_interval(segment,&interval,&pending,envelope,record,&state,reference))return -1;
                    }
                    run_start=run_end=j+1;interval.maximum=4.0f;
                    if(state.transitions>=7)break;
                }else{
                    ++run_end;if(interval.maximum<envelope[j])interval.maximum=envelope[j];
                }
            }
            if(state.transitions<7&&run_end-run_start>minimum){
                if(run_start>0&&run_start<=32){
                    interval.start=run_start;interval.end=run_end;
                    if(sony_recovered_plus_insert_gain_interval(segment,&interval,&pending,envelope,record,&state,reference))return -1;
                }else if(request->allow_head&&request->head_state==0&&original_transitions>0&&run_start==0&&segment->end<32){
                    int32_t first=0,end=0;float maximum=envelope[0];
                    for(int32_t j=0;j<=segment->end;++j){
                        if((double)threshold*(double)envelope[j]>(double)segment->baseline){
                            if(end-first>minimum&&enqueue(&pending,first,end,maximum,classification))return -1;
                            first=end=j+1;maximum=4.0f;
                        }else{
                            ++end;if(maximum<envelope[j])maximum=envelope[j];
                        }
                    }
                }
            }
        }
        if(sony_recovered_plus_refine_gain_intervals(record,&state,&pending,envelope,reference))return -1;
        if(state.candidate_count){
            baseline.selected=record->selected_baseline;baseline.state=record->baseline_state;
            if(sony_recovered_plus_refine_gain_baseline(&baseline,state.candidate,state.candidate_count,last))return -1;
            record->selected_baseline=baseline.selected;record->baseline_state=baseline.state;
        }
    }
    const int32_t result=sony_recovered_plus_prune_gain_candidates(state.levels,state.candidate,state.candidate_count,state.differences);
    if(result<0)return -1;
    if(sony_recovered_plus_weighted_gain_maximum(envelope,state.levels,&record->weighted_maximum))return -1;
    memcpy(levels,state.levels,33*sizeof(int32_t));return 0;
}
