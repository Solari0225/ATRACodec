#include "independent_plus_gain_band.h"
#include "independent_plus_gain_helpers.h"
#include "independent_plus_gain_search.h"
#include "independent_plus_gain_profile.h"
#include "independent_plus_gain_log.h"
#include <stddef.h>
#include <string.h>
#include "gain_segments_tables.inc"
_Static_assert(sizeof(sony_plus_gain_record)==152,"gain record numeric layout");
_Static_assert(offsetof(sony_plus_gain_record,maximum)==0x48,"gain maximum offset");
_Static_assert(offsetof(sony_plus_gain_record,difference_envelope_sum)==0x94,"gain sum offset");
typedef struct envelope_analysis {float value[65],paired[36];int32_t peak,past_peak;} envelope_analysis;
static float spill(double value){volatile float result=(float)value;return result;}
static float number(uint32_t bits){float result;memcpy(&result,&bits,4);return result;}
static float absolute(float value){uint32_t bits;memcpy(&bits,&value,4);bits&=0x7fffffffu;return number(bits);}
static double power(int32_t exponent){uint32_t word=1u<<((exponent<0?0u-(uint32_t)exponent:(uint32_t)exponent)&31u);int32_t base;memcpy(&base,&word,4);return exponent<0?1.0/(double)base:(double)base;}
static int32_t integer(double value){
    if(!(value>=-0x1p63&&value<0x1p63))return 0;
    const int64_t wide=(int64_t)value;const uint32_t word=(uint32_t)(uint64_t)wide;int32_t result;memcpy(&result,&word,4);return result;
}
static double logarithm(double ratio){uint64_t word;memcpy(&word,&ratio,8);return word==UINT64_C(0x7ff0000000000000)?ratio:sony_recovered_plus_gain_log_measure(ratio);}
static float maximum(const float *values,uint32_t count){return sony_recovered_plus_gain_maximum(values,(int32_t)count);}
static void pair_envelope(envelope_analysis *analysis){
    float peak=0.0f;analysis->peak=analysis->past_peak=0;
    for(int32_t i=0;i<36;i+=2){
        const int32_t selected=analysis->value[i]<analysis->value[i+1]?i+1:i;
        const float value=analysis->value[selected];analysis->paired[i]=analysis->paired[i+1]=value;
        if(peak<value){peak=value;analysis->peak=selected;if(i<32)analysis->past_peak=selected;}
    }
}
static float forward_floor(float lower,float upper,float previous){
    float result=lower;const double bound=(double)lower*1.5;
    if((double)previous<=bound){result=previous;if(lower<previous&&bound<(double)upper)result=lower;}
    return result;
}
static int32_t sum_increments(const sony_plus_gain_search_points *points){int32_t sum=0;for(int32_t i=0;i<points->count;++i)sum+=points->increment[i];return sum;}
static int32_t backward_limit(int32_t rise,const sony_plus_gain_record *previous){const int32_t limit=rise+previous->rise_sum-previous->fall_sum;return limit<0?limit+6:6;}
static void curve(const sony_plus_gain_search_points *forward,const sony_plus_gain_search_points *backward,int32_t values[33]){
    int32_t rising[7],falling[7],sum=0;for(int32_t i=forward->count-1;i>=0;--i){sum+=forward->increment[i];rising[i]=sum;}
    sum=0;for(int32_t i=backward->count-1;i>=0;--i){sum+=backward->increment[i];falling[i]=sum;}
    for(uint32_t i=0;i<33;++i)values[i]=0;int32_t at=0;
    for(int32_t i=0;i<forward->count;++i)while(at<=forward->location[i]&&at<33)values[at++]+=rising[i];
    at=32;for(int32_t i=0;i<backward->count;++i)while(at>=backward->location[i]&&at>=0)values[at--]+=falling[i];
    const int32_t last=values[32];for(uint32_t i=0;i<33;++i)values[i]-=last;
}
static void membership(const sony_plus_gain_search_points *points,int32_t falling,int32_t output[33]){
    int32_t count=0;
    for(int32_t i=falling?32:0;falling?i>=0:i<33;i+=falling?-1:1){
        output[i]=count;if(count<points->count&&points->location[count]==i){output[i]=count+8;++count;}
    }
}
static float weighted_sum(const int32_t levels[33],const float envelope[65],int32_t begin,int32_t end){
    float result=0.0f;for(int32_t i=begin;i<end;++i)result=spill(power(levels[i])*(double)envelope[i]+(double)result);return result;
}
static void insert_point(sony_plus_gain_search_points *points,int32_t index,int32_t position,int32_t increment){
    for(int32_t i=points->count;i>index;--i){points->location[i]=points->location[i-1];points->increment[i]=points->increment[i-1];}
    points->location[index]=position;points->increment[index]=increment;++points->count;
}
static void promote_difference_points(sony_plus_gain_search_points *forward,sony_plus_gain_search_points *backward,
    const sony_plus_gain_search_points *difference,const float envelope[65],int32_t falling,int32_t band,
    float envelope_sum,float difference_sum,int32_t limit,int32_t *used){
    if(difference->count<=0||forward->count+backward->count>=7)return;
    sony_plus_gain_search_points *target=falling?backward:forward;
    int32_t occupied[33],levels[33];membership(target,falling,occupied);curve(forward,backward,levels);
    for(int32_t point=0;point<difference->count;++point){
        const int32_t position=difference->location[point],index=occupied[position];
        if(index<8&&(position<1||occupied[position-1]<8)&&(position>30||occupied[position+1]<8)){
            if(!falling&&band==0&&envelope_sum>100000.0f&&(double)envelope_sum>(double)difference_sum*10.0)continue;
            const int32_t increment=difference->increment[point];double left,right;int accepted;
            if(!falling){
                const float first=weighted_sum(levels,envelope,0,position+1),second=weighted_sum(levels,envelope,position+1,33);
                const float average=spill((double)first/(double)(position+1));
                left=power(increment)*(double)average;
                right=((double)second/(double)(32-position))*(double)number(0x3fb4fdf4u);accepted=left<right;
            }else{
                float first=weighted_sum(levels,envelope,0,position);const float second=weighted_sum(levels,envelope,position,33);
                if(position>0)first=spill((double)first/(double)position);
                left=((double)second/(double)(33-position))*power(increment);right=first;accepted=left<right;
            }
            if(accepted){
                int32_t amount=increment;if(limit<amount+*used)amount=limit-*used;*used+=amount;
                if(amount>0){
                    insert_point(target,index,position,amount);occupied[position]=index+8;
                    if(falling){for(int32_t j=position;j>=0;--j)++occupied[j];}
                    else for(int32_t j=position+1;j<32;++j)++occupied[j];
                }
            }
        }
        if(forward->count+backward->count>=7)break;
    }
}
static void append_boundary(sony_plus_gain_search_points *forward,const sony_plus_gain_search_points *backward,
    float boundary,float first_peak,int32_t freeze,int32_t difference_mode,int32_t retain_fraction){
    if(backward->count<=0||!freeze||!((double)first_peak*1.75<(double)boundary))return;
    const double measure=logarithm((double)boundary/(double)first_peak);
    double bias=0.5;if(retain_fraction&&difference_mode&&forward->count&&measure>=2.0)bias=forward->fraction[forward->count-1];
    const int32_t increment=integer(measure+bias);const float fraction=spill(measure-(double)increment);
    if(increment>0){
        if(forward->count>0&&forward->location[forward->count-1]==31){forward->increment[forward->count-1]=increment;if(retain_fraction&&difference_mode)forward->fraction[forward->count-1]=fraction;}
        else if(forward->count+backward->count<7){const int32_t i=forward->count++;forward->location[i]=31;forward->increment[i]=increment;if(retain_fraction&&difference_mode)forward->fraction[i]=fraction;}
    }
}
static void reset_points(sony_plus_gain_record *record,int32_t band,int32_t *error){
    record->points.count=0;for(uint32_t i=0;i<7;++i){record->points.location[i]=0;record->points.level[i]=0;}
    record->rise_sum=record->fall_sum=record->forward_count=record->first_forward_location=record->last_backward_location=record->baseline_state=0;
    record->weighted_maximum=record->maximum;
    const double factor=(double)number(gain_baseline_band_scale[band])*(double)record->low_band_scale;
    record->selected_baseline=record->upper_maximum<record->maximum&&(double)record->maximum<factor*(double)record->upper_maximum?record->upper_maximum:record->maximum;
    record->difference_rise_sum=record->difference_fall_sum=0;
    record->difference_baseline=record->difference_upper_maximum<record->difference_maximum&&
        (double)record->difference_maximum<factor*(double)record->difference_upper_maximum?record->difference_upper_maximum:record->difference_maximum;
    *error=142;
}
int sony_recovered_plus_select_gain_band(const sony_plus_gain_band_request *request,
    const float input[133],sony_plus_gain_band_history *history,
    const sony_plus_gain_record previous[16],sony_plus_gain_record current[16],int32_t *error){
    if(!request||!input||!history||!previous||!current||!error||request->band<0||request->band>15||request->channels<1||request->channels>2)return -1;
    const int32_t band=request->band,difference_mode=request->mode!=2;
    const sony_plus_gain_record *old=previous+band;sony_plus_gain_record *now=current+band;
    if(old->points.count>7)return -1;
    envelope_analysis amplitude={0},difference={0};
    memcpy(amplitude.value,history->envelope,(difference_mode?33u:32u)*sizeof(float));
    double new_sum=0.0;
    for(uint32_t block=0;block<32;++block){float peak=4.0f;for(uint32_t lane=0;lane<4;++lane){const float v=absolute(input[1+block*4+lane]);if(peak<v)peak=v;}amplitude.value[32+block]=peak;new_sum+=(double)peak;}
    float lookahead=amplitude.value[63];
    if(difference_mode){lookahead=0.0f;for(uint32_t i=129;i<133;++i){const float v=absolute(input[i]);if(lookahead<v)lookahead=v;}if(lookahead<4.0f)lookahead=4.0f;}
    amplitude.value[64]=lookahead;memcpy(history->envelope,amplitude.value+32,(difference_mode?33u:32u)*sizeof(float));
    const int32_t new_turns=sony_recovered_plus_gain_envelope_turns(amplitude.value[31],amplitude.value+32);
    const int32_t transient=old->previous_turns>=4||old->current_turns>=4||new_turns>=4;
    if(difference_mode){
        float samples[128];for(uint32_t i=0;i<128;++i)samples[i]=spill(((double)input[i]-(double)input[i+1])*0.5);
        memcpy(difference.value,history->difference,32*sizeof(float));double sum=0.0;
        for(uint32_t block=0;block<32;++block){float peak=4.0f;for(uint32_t lane=0;lane<4;++lane){const float v=absolute(samples[block*4+lane]);if(peak<v)peak=v;}difference.value[32+block]=peak;sum+=(double)peak;}
        difference.value[64]=4.0f;memcpy(history->difference,difference.value+32,32*sizeof(float));
        now->envelope_sum=spill(new_sum);now->difference_envelope_sum=spill(sum);
    }
    pair_envelope(&amplitude);if(difference_mode)pair_envelope(&difference);
    const float old_lower=maximum(amplitude.paired,16),old_upper=maximum(amplitude.paired+16,16);
    now->maximum=old_lower<old_upper?old_upper:old_lower;now->upper_maximum=old_upper;
    const float secondary=forward_floor(old_lower,old_upper,old->selected_baseline);
    const float initial=old->points.count?old->selected_baseline:secondary;
    int32_t limit=integer(logarithm(65536.0/(double)old->weighted_maximum));
    const int32_t unity=sony_recovered_plus_gain_above_unity(&old->points);
    sony_plus_gain_search_points forward={0},backward={0},dforward={0},dbackward={0};
    sony_plus_gain_forward_state fs={old->baseline_state,unity,limit,0},dfs={0,unity,limit,0};
    float threshold=spill((double)now->low_band_scale*1.5);
    if(request->max_qu<27&&band==0&&now->peak_ratio>=10.0f)threshold=spill((double)threshold*1.5);
    float d_old_upper=4.0f,d_initial=4.0f,d_secondary=4.0f;
    if(difference_mode){const float lower=maximum(difference.paired,16);d_old_upper=maximum(difference.paired+16,16);now->difference_maximum=lower<d_old_upper?d_old_upper:lower;now->difference_upper_maximum=d_old_upper;d_secondary=forward_floor(lower,d_old_upper,old->difference_baseline);d_initial=old->points.count?old->difference_baseline:d_secondary;}
    sony_plus_gain_forward_request fr={amplitude.peak<32?amplitude.peak:32,1,0,0,difference_mode,initial,secondary,threshold};
    forward.count=sony_recovered_plus_search_gain_forward(&fr,&fs,amplitude.value,&forward);if(forward.count<0)return -1;
    if(band==0&&now->low_band_scale==1.0f&&now->peak_ratio<=15.0f){
        const sony_plus_gain_record *neighbor=current+1;
        if(neighbor->forward_count>0&&neighbor->rise_sum>=2&&neighbor->first_forward_location<neighbor->last_backward_location&&fs.total_delta<limit&&now->maximum>neighbor->maximum&&forward.count==0){forward.increment[0]=1;forward.location[0]=neighbor->first_forward_location;forward.count=1;}
        if((forward.count==1||forward.count==2)&&now->maximum>neighbor->maximum&&fs.used_delta<limit&&sum_increments(&forward)<=2&&neighbor->rise_sum>=4){++forward.increment[0];if(forward.count==1)forward.location[0]=neighbor->first_forward_location;}
    }
    if(difference_mode){
        sony_plus_gain_forward_request dr={difference.peak<32?difference.peak:32,1,0,0,difference_mode,d_initial,d_secondary,threshold};
        dforward.count=sony_recovered_plus_search_gain_forward(&dr,&dfs,difference.value,&dforward);if(dforward.count<0)return -1;
        now->difference_rise_sum=sum_increments(&dforward);
        if(band==0&&now->low_band_scale>1.0f&&forward.count==0&&dforward.count==1){
            int compatible=1;for(int32_t i=1;i<=3;++i)if(current[i].points.count==0||current[i].points.location[current[i].points.count-1]!=(uint32_t)dforward.location[0])compatible=0;
            if(compatible){forward.count=1;forward.increment[0]=dforward.increment[0];forward.location[0]=dforward.location[0];forward.fraction[0]=dforward.fraction[0];}
        }
    }
    now->forward_count=forward.count;if(forward.count)now->first_forward_location=forward.location[0];now->rise_sum=sum_increments(&forward);
    const float current_lower=maximum(amplitude.value+32,16),current_upper=maximum(amplitude.value+48,16);
    float candidate=current_lower<current_upper?current_upper:current_lower;
    if((double)current_lower*1.75<(double)candidate)candidate=current_lower;
    const int32_t freeze=(double)old_upper*1.75<(double)candidate;
    const float back_initial=freeze?old_upper:candidate;
    sony_plus_gain_backward_state bs={0,backward_limit(now->rise_sum,old),back_initial},dbs={0,6,4.0f};
    const float back_threshold=spill((double)now->low_band_scale*1.75);
    if(forward.count>0||now->peak_ratio<=15.0f){sony_plus_gain_backward_request br={amplitude.past_peak,2,forward.count,0,freeze,difference_mode,back_initial,back_threshold};backward.count=sony_recovered_plus_search_gain_backward(&br,&bs,amplitude.paired,amplitude.value,&backward);if(backward.count<0)return -1;}
    int32_t d_freeze=0;float d_back_initial=4.0f;
    if(difference_mode){
        const float lower=maximum(difference.value+32,16),upper=maximum(difference.value+48,16);float c=lower<upper?upper:lower;
        if((double)lower*1.75<(double)c)c=lower;d_freeze=(double)d_old_upper*1.75<(double)c;d_back_initial=d_freeze?d_old_upper:c;
        const int32_t dl=now->difference_rise_sum+old->difference_rise_sum-old->difference_fall_sum;
        dbs.limit=dl<0?dl+6:6;dbs.first_peak=d_back_initial;
        if(dforward.count>0||now->peak_ratio<=15.0f){sony_plus_gain_backward_request br={difference.past_peak,2,dforward.count,0,d_freeze,difference_mode,d_back_initial,back_threshold};dbackward.count=sony_recovered_plus_search_gain_backward(&br,&dbs,difference.paired,difference.value,&dbackward);if(dbackward.count<0)return -1;}
    }
    append_boundary(&forward,&backward,amplitude.value[32],bs.first_peak,freeze,difference_mode,1);
    int empty_fallback=0;
    if(!forward.count){
        float t=spill((double)now->low_band_scale*1.5);if(band==0)t=spill((double)t*2.0);
        fs.limit=bs.limit;fs.total_delta=bs.used_delta;
        sony_plus_gain_forward_request fallback={amplitude.peak<30?amplitude.peak:30,2,0,backward.count,difference_mode,initial,-1.0f,t};
        forward.count=sony_recovered_plus_search_gain_forward(&fallback,&fs,amplitude.paired,&forward);if(forward.count<0)return -1;bs.used_delta=fs.total_delta;empty_fallback=forward.count==0;
    }
    if(!empty_fallback){now->rise_sum=sum_increments(&forward);now->forward_count=forward.count;now->first_forward_location=forward.location[0];bs.limit=backward_limit(now->rise_sum,old);bs.used_delta=0;}
    if(!backward.count&&now->peak_ratio<=15.0f){
        float grouped[36];for(uint32_t i=0;i<36;i+=4){const float p=amplitude.paired[i]<amplitude.paired[i+2]?amplitude.paired[i+2]:amplitude.paired[i];for(uint32_t j=0;j<4;++j)grouped[i+j]=p;}
        float t=spill((double)now->low_band_scale*1.75);if(band==0)t=spill((double)t*2.0);
        sony_plus_gain_backward_request br={amplitude.past_peak,4,forward.count,0,freeze,difference_mode,back_initial,t};backward.count=sony_recovered_plus_search_gain_backward(&br,&bs,grouped,amplitude.value,&backward);if(backward.count<0)return -1;
    }
    now->last_backward_location=backward.count?backward.location[backward.count-1]:31;now->fall_sum=sum_increments(&backward);
    int32_t head_state=0;
    if(difference_mode){
        append_boundary(&dforward,&dbackward,difference.value[32],dbs.first_peak,d_freeze,0,0);
        head_state=forward.count+backward.count;
        promote_difference_points(&forward,&backward,&dforward,amplitude.value,0,band,old->envelope_sum,old->difference_envelope_sum,bs.limit,&fs.used_delta);
        promote_difference_points(&forward,&backward,&dbackward,amplitude.value,1,band,old->envelope_sum,old->difference_envelope_sum,bs.limit,&bs.used_delta);
    }
    if(band<request->coherence_start&&!forward.count&&backward.count==1&&backward.increment[0]>4){
        const int32_t location=backward.location[0];
        if(location>0&&location<=36&&(double)current_lower*16.0<(double)amplitude.paired[location-1]&&(double)current_upper*16.0<(double)amplitude.paired[location-1]){
            const int32_t end=location+32;int found=0;
            for(int32_t probe=location+1;probe<end;++probe)if(amplitude.value[probe]>=10.0f){found=1;break;}
            if(found)backward.increment[0]=4;
        }
    }
    int32_t levels[33],difference_levels[33];curve(&forward,&backward,levels);if(difference_mode)curve(&dforward,&dbackward,difference_levels);
    const int32_t resolution=band<request->upper_band&&((request->channels==2&&request->max_qu>11&&request->max_qu<=15)||(request->channels==1&&request->max_qu<=15))?2:1;
    sony_plus_gain_refinement_record rr={now->maximum,now->upper_maximum,now->low_band_scale,now->weighted_maximum,now->selected_baseline,(uint32_t)now->baseline_state};
    sony_plus_gain_profile_request profile={(uint32_t)band,initial,back_initial,bs.first_peak,transient,resolution,head_state,difference_mode};
    if(sony_recovered_plus_refine_gain_profile(&rr,amplitude.value,levels,&profile))return -1;
    now->weighted_maximum=rr.weighted_maximum;now->selected_baseline=rr.selected_baseline;now->baseline_state=(int32_t)rr.baseline_state;
    if(difference_mode){
        int32_t differences[32],count=0;for(uint32_t i=0;i<32;++i){differences[i]=difference_levels[i]-difference_levels[i+1];if(differences[i])++count;}
        sony_plus_gain_baseline baseline={now->difference_maximum,now->difference_upper_maximum,now->low_band_scale,now->difference_baseline,0};
        if(sony_recovered_plus_gain_baseline_difference(&baseline,difference.value,differences,(uint32_t)band,dbs.first_peak,count)<0)return -1;now->difference_baseline=baseline.selected;
    }
    uint32_t count=0;for(uint32_t i=0;i<32;++i)if(levels[i]!=levels[i+1]){
        if(count>=7){reset_points(now,band,error);goto finish;}
        now->points.location[count]=i;now->points.level[count]=(uint32_t)sony_recovered_plus_gain_level_index(levels[i]);++count;
    }
    now->points.count=count;
finish:
    now->edge_maximum=amplitude.paired[31];now->previous_turns=old->current_turns;now->current_turns=new_turns;return 0;
}
