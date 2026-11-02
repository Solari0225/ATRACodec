/* Additive full0..15 tone-count domain; frozen v1 retained unchanged. */
#include "independent_plus_tone_cost_v2.h"
#include "../plus_mode_cost/independent_plus_flag_cost.h"
typedef struct tone_huff {uint16_t code;uint8_t width;} tone_huff;
#include "../plus_fields/tone_math.inc"
static int valid(const plus_tone_mode_state* s){
    if(!s||s->band_count<0||s->band_count>16)return 0;
    for(unsigned ch=0;ch<2;ch++){
        if(s->channels[ch].channel_index>1)return 0;
        for(int32_t b=0;b<s->band_count;b++){
            const plus_given_tone_band* p=s->channels[ch].bands+b;
            if(p->count>15)return 0;
            for(uint32_t i=0;i<p->count;i++)if(p->tones[i].frequency>1023||p->tones[i].amplitude>63||p->tones[i].fine_amplitude>15)return 0;
        }
    }return 1;
}
static uint32_t width(uint32_t book,uint32_t symbol){return tone_codes[tone_book_starts[book]+symbol].width;}
static uint32_t ascending(const plus_given_tone_band* b){
    uint32_t cost=b->count?10:0;
    for(uint32_t i=1;i<b->count;i++){
        uint32_t n=10;int32_t previous=(int32_t)b->tones[i-1].frequency;
        while(n>1&&previous>=(int32_t)(1024-(1u<<(n-1))))n--;
        cost+=n;
    }return cost;
}
static uint32_t descending(const plus_given_tone_band* b){
    uint32_t cost=b->count?10:0;
    for(uint32_t i=1;i<b->count;i++){
        uint32_t n=1;int32_t next=(int32_t)b->tones[i].frequency;
        while(n<10&&next>=(int32_t)(1u<<n))n++;
        cost+=n;
    }return cost;
}
int32_t plus_tone_prepare_selected_v2(plus_tone_mode_state* s,uint32_t channel){
    if(!valid(s)||channel>1)return PLUS_PACK_INVALID;
    plus_given_tone_channel* c=s->channels+channel;
    for(int32_t b=0;b<s->band_count;b++)c->selected[b]=c->channel_index?s->flags[0][b+2]==0:1;
    return 0;
}
int32_t plus_tone_prepare_stereo_map_v2(plus_tone_mode_state* s,uint32_t channel){
    if(!valid(s)||channel>1)return PLUS_PACK_INVALID;
    const plus_given_tone_channel* c=s->channels+channel;const plus_given_tone_channel* ref=s->channels+(1-channel);uint32_t flat=0;
    for(int32_t b=0;b<s->band_count;b++)if(c->selected[b]){
        const plus_given_tone_band* p=c->bands+b;const plus_given_tone_band* q=ref->bands+b;
        for(uint32_t i=0;i<p->count;i++){
            uint32_t distance=1024,index=0;
            for(uint32_t j=0;j<q->count;j++){
                int32_t d=(int32_t)p->tones[i].frequency-(int32_t)q->tones[j].frequency;
                uint32_t absolute=d<0?(uint32_t)-d:(uint32_t)d;
                if(absolute<distance){distance=absolute;index=j;}
            }
            s->stereo_map[flat++]=distance<8?(int32_t)index:i<q->count?(int32_t)i:-1;
        }
    }
    return 0;
}
static uint32_t envelope_cost(const plus_given_tone_band* p,const plus_given_tone_band* q,uint32_t mode){
    if(!mode)return 2+(p->envelope[0]?5:0)+(p->envelope[1]?5:0);
    for(uint32_t i=0;i<4;i++)if(p->envelope[i]!=q->envelope[i])return 0x4000;
    return 0;
}
static uint32_t count_cost(const plus_given_tone_band* p,const plus_given_tone_band* q,uint32_t mode){
    if(!mode)return 4;
    if(mode==1)return p->count<8?width(1,p->count):0x4000;
    if(mode==3)return p->count==q->count?0:0x4000;
    int32_t delta=(int32_t)p->count-(int32_t)q->count;
    return delta>=-4&&delta<4?width(2,(uint32_t)delta&7):0x4000;
}
static uint32_t frequency_delta(const plus_given_tone_band* p,const plus_given_tone_band* q){
    uint32_t cost=0;
    for(uint32_t i=0;i<p->count;i++){
        uint32_t previous=q->count?q->tones[i<q->count?i:q->count-1].frequency:0;
        int32_t delta=(int32_t)p->tones[i].frequency-(int32_t)previous;
        if(delta< -128||delta>=128)return 0x4000;
        cost+=width(3,(uint32_t)delta&255);
    }return cost;
}
static uint32_t amplitude_cost(const plus_tone_mode_state* s,const plus_given_tone_band* p,
    const plus_given_tone_band* q,uint32_t flat,uint32_t mode,int fine){
    const uint32_t count=fine||s->amplitude_mode?p->count:p->count!=0;
    if(!mode)return count*(fine?4:6);
    uint32_t cost=0;
    for(uint32_t i=0;i<count;i++){
        uint32_t value=fine?p->tones[i].fine_amplitude:p->tones[i].amplitude;
        if(mode==1){
            if(fine)cost+=width(p->count==1?7:8,value);
            else{
                uint32_t index=value-(s->amplitude_mode?20u:24u);
                if(index>=32)return 0x4000;
                cost+=width(s->amplitude_mode?5:4,index);
            }continue;
        }
        uint32_t previous;
        if(!fine&&!s->amplitude_mode)previous=q->count?q->tones[0].amplitude:(mode==3?49u:44u);
        else{
            int32_t index=s->stereo_map[flat+i];
            if(index>=16)return (uint32_t)PLUS_PACK_INVALID;
            previous=index>=0?(fine?q->tones[index].fine_amplitude:q->tones[index].amplitude):fine?(mode==3?14u:12u):(mode==3?32u:34u);
        }
        if(mode==3){if(value!=previous)return 0x4000;}
        else{
            int32_t delta=(int32_t)value-(int32_t)previous;
            int32_t limit=fine?4:16;
            if(delta< -limit||delta>=limit)return 0x4000;
            cost+=width(fine?9:6,(uint32_t)delta&(fine?7u:31u));
        }
    }return cost;
}
int32_t plus_tone_mode_candidate_v2(plus_tone_mode_state* s,uint32_t channel,plus_tone_substage component,uint32_t mode){
    if(!valid(s)||channel>1||mode>3||((component==PLUS_TONE_ENVELOPE||component==PLUS_TONE_FREQUENCY)&&mode>1))return PLUS_PACK_INVALID;
    plus_given_tone_channel* c=s->channels+channel;const plus_given_tone_channel* ref=s->channels+(1-channel);uint32_t cost=0,flat=0;
    for(int32_t b=0;b<s->band_count;b++)if(c->selected[b]){
        const plus_given_tone_band* p=c->bands+b;const plus_given_tone_band* q=ref->bands+b;
        uint32_t value;
        if(component==PLUS_TONE_ENVELOPE)value=envelope_cost(p,q,mode);
        else if(component==PLUS_TONE_COUNT)value=count_cost(p,q,mode);
        else if(component==PLUS_TONE_FREQUENCY){
            if(mode)value=frequency_delta(p,q);
            else{
                value=ascending(p);
                if(p->count>1){uint32_t reverse=descending(p);c->frequency_direction[b]=value>reverse;if(reverse<value)value=reverse;value++;}
            }
        }else if(component==PLUS_TONE_AMPLITUDE||component==PLUS_TONE_FINE_AMPLITUDE){
            value=amplitude_cost(s,p,q,flat,mode,component==PLUS_TONE_FINE_AMPLITUDE);flat+=p->count;
            if(value==(uint32_t)PLUS_PACK_INVALID)return PLUS_PACK_INVALID;
        }else if(component==PLUS_TONE_PHASE)value=p->count*5;
        else return PLUS_PACK_INVALID;
        cost+=value;
    }
    return (int32_t)cost;
}
int32_t plus_tone_choose_component_v2(plus_tone_mode_state* s,uint32_t channel,plus_tone_substage component){
    if(!valid(s)||channel>1||component>=PLUS_TONE_PHASE)return PLUS_PACK_INVALID;
    plus_given_tone_channel* c=s->channels+channel;
    uint32_t n=(component==PLUS_TONE_ENVELOPE||component==PLUS_TONE_FREQUENCY)?(c->channel_index?2:1):(c->channel_index?4:2),best=0;
    int32_t costs[4];
    for(uint32_t mode=0;mode<n;mode++){costs[mode]=plus_tone_mode_candidate_v2(s,channel,component,mode);if(costs[mode]<0)return costs[mode];if(costs[mode]<costs[best])best=mode;}
    if(component==PLUS_TONE_ENVELOPE)c->envelope_mode=best;
    else if(component==PLUS_TONE_COUNT)c->count_mode=best;
    else if(component==PLUS_TONE_FREQUENCY)c->frequency_mode=best;
    else if(component==PLUS_TONE_AMPLITUDE)c->amplitude_mode=best;
    else c->fine_amplitude_mode=best;
    return costs[best];
}
int32_t plus_tone_choose_channel_v2(plus_tone_mode_state* s,uint32_t channel,int32_t choose){
    if(plus_tone_prepare_selected_v2(s,channel)<0)return PLUS_PACK_INVALID;
    plus_given_tone_channel* c=s->channels+channel;uint32_t cost=0;
    for(int32_t component=PLUS_TONE_ENVELOPE;component<=PLUS_TONE_PHASE;component++){
        if(component==PLUS_TONE_FINE_AMPLITUDE&&s->amplitude_mode)continue;
        if(choose==1&&component==PLUS_TONE_AMPLITUDE&&c->channel_index==1)if(plus_tone_prepare_stereo_map_v2(s,channel)<0)return PLUS_PACK_INVALID;
        int32_t value=choose==1&&component!=PLUS_TONE_PHASE?plus_tone_choose_component_v2(s,channel,(plus_tone_substage)component):plus_tone_mode_candidate_v2(s,channel,(plus_tone_substage)component,0);
        if(value<0)return value;cost+=(uint32_t)value;
    }
    if(choose!=1)c->envelope_mode=c->count_mode=c->frequency_mode=c->amplitude_mode=c->fine_amplitude_mode=0;
    return (int32_t)(cost+(c->channel_index?(s->amplitude_mode?6:8):(s->amplitude_mode?2:3)));
}
int32_t plus_tone_choose_modes_v2(plus_tone_mode_state* s,int32_t choose){
    if(!s)return PLUS_PACK_INVALID;
    if(!s->present)return 1;
    if(!valid(s)||s->band_count<1||s->channel_count<1||s->channel_count>2)return PLUS_PACK_INVALID;
    for(int32_t b=0;b<s->band_count;b++){
        if(s->channel_count==2&&!s->channels[0].bands[b].count&&s->channels[1].bands[b].count){
            plus_given_tone_band temporary=s->channels[0].bands[b];s->channels[0].bands[b]=s->channels[1].bands[b];s->channels[1].bands[b]=temporary;
            for(uint32_t i=0;i<4;i++){uint32_t v=s->previous_envelope[0][b][i];s->previous_envelope[0][b][i]=s->previous_envelope[1][b][i];s->previous_envelope[1][b][i]=v;}
            s->flags[1][b+2]=1;
        }else s->flags[1][b+2]=0;
    }
    uint32_t cost=2+width(0,(uint32_t)s->band_count-1);
    if(s->channel_count==2)for(uint32_t i=0;i<3;i++){
        plus_given_flag_vector v={s->flags[i][0],s->flags[i][1],s->flags[i]+2};int32_t bits=plus_mode_choose_flags(&v,s->band_count);
        if(bits<0)return bits;s->flags[i][0]=v.present;s->flags[i][1]=v.explicit_values;cost+=(uint32_t)bits;
    }
    for(int32_t ch=0;ch<s->channel_count;ch++){int32_t bits=plus_tone_choose_channel_v2(s,(uint32_t)ch,choose);if(bits<0)return bits;cost+=(uint32_t)bits;}
    return (int32_t)cost;
}
