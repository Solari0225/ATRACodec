#include "independent_plus_gain_cost.h"
typedef struct plus_gain_huff {uint16_t code;uint8_t width;} plus_gain_huff;
#include "../plus_fields/gain_huffman.inc"
#include "../plus_fields/gain_location_huffman.inc"
#include "gain_width_math.inc"
static int valid(const plus_given_gain_channel* c){
    if(!c||c->channel_index>1||c->num_bands<1||c->num_bands>16)return 0;
    for(uint32_t b=0;b<c->num_bands;b++){
        const plus_given_gain_band* p=c->bands+b;if(p->points>7)return 0;
        for(uint32_t i=0;i<p->points;i++)if(p->levels[i]>15||p->locations[i]>31||(i&&p->locations[i]<=p->locations[i-1]))return 0;
    }return 1;
}
static uint32_t within_levels(const plus_given_gain_band* b){
    uint32_t cost=b->points?gain_level_absolute[b->levels[0]].width:0;
    for(uint32_t i=1;i<b->points;i++)cost+=gain_level_within_band_delta[(b->levels[i]-b->levels[i-1])&15].width;
    return cost;
}
static uint32_t direct_location(const plus_given_gain_band* b,uint32_t point){return point?(uint32_t)gain_location_width[b->locations[point-1]]:5;}
static uint32_t within_locations(const plus_given_gain_band* b){
    uint32_t cost=b->points?5:0;
    for(uint32_t i=1;i<b->points;i++){
        const plus_gain_huff* h=b->levels[i]>b->levels[i-1]?gain_location_within_rise:gain_location_within_nonrise;
        cost+=h[b->locations[i]-b->locations[i-1]].width;
    }return cost;
}
static int32_t count_cost(plus_given_gain_channel* c,const plus_given_gain_channel* ref,uint32_t mode){
    uint32_t cost=0;
    if(mode==3&&!c->channel_index){
        uint32_t minimum=7,maximum=0;
        for(uint32_t b=0;b<c->num_bands;b++){uint32_t n=c->bands[b].points;if(n<minimum)minimum=n;if(n>maximum)maximum=n;}
        c->count_base=minimum;c->count_width=(uint32_t)gain_count_width[maximum-minimum];
        return (int32_t)(5+c->num_bands*c->count_width);
    }
    for(uint32_t b=0;b<c->num_bands;b++){
        const uint32_t n=c->bands[b].points;
        if(mode==0)cost+=3;
        else if(mode==1||(mode==2&&!c->channel_index&&!b))cost+=gain_count_absolute[n].width;
        else if(mode==2){uint32_t previous=c->channel_index?ref->bands[b].points:c->bands[b-1].points;cost+=gain_count_delta[(n-previous)&7].width;}
        else if(n!=ref->bands[b].points)return 0x4000;
    }return (int32_t)cost;
}
static int32_t level_cost(plus_given_gain_channel* c,const plus_given_gain_channel* ref,uint32_t mode){
    uint32_t cost=0;
    if(mode==3&&!c->channel_index){
        uint32_t minimum=15,maximum=0,points=0;
        for(uint32_t b=0;b<c->num_bands;b++)for(uint32_t i=0;i<c->bands[b].points;i++){
            uint32_t v=c->bands[b].levels[i];if(v<minimum)minimum=v;if(v>maximum)maximum=v;points++;
        }
        if(!points)return PLUS_PACK_INVALID;
        c->level_base=minimum;c->level_width=(uint32_t)gain_level_width[maximum-minimum];
        return (int32_t)(6+points*c->level_width);
    }
    for(uint32_t b=0;b<c->num_bands;b++){
        const plus_given_gain_band* p=c->bands+b;
        if(mode==0){cost+=p->points*4;continue;}
        if(mode==1&&!c->channel_index){cost+=within_levels(p);continue;}
        if(mode==2&&c->channel_index){
            c->level_explicit[b]=1;
            if(p->points){
                uint32_t different=0;
                for(uint32_t i=0;i<p->points;i++)if(p->levels[i]!=(i<ref->bands[b].points?ref->bands[b].levels[i]:7))different=1;
                c->level_explicit[b]=different;cost++;
                if(different)cost+=within_levels(p);
            }continue;
        }
        if(mode==2&&!c->channel_index&&!b){cost+=within_levels(p);continue;}
        const plus_given_gain_band* previous=c->channel_index?ref->bands+b:c->bands+b-1;
        for(uint32_t i=0;i<p->points;i++){
            uint32_t value=i<previous->points?previous->levels[i]:7;
            if(mode==3){if(p->levels[i]!=value)return 0x4000;}
            else cost+=(c->channel_index?gain_level_stereo_delta:gain_level_band_delta)[(p->levels[i]-value)&15].width;
        }
    }return (int32_t)cost;
}
static uint32_t predicted_locations(const plus_given_gain_band* b,const plus_given_gain_band* p,int stereo){
    uint32_t cost=0;
    if(b->points){
        uint32_t previous=p->points?p->locations[0]:0;
        cost+=(stereo?gain_location_stereo:gain_location_band_nonrise)[(b->locations[0]-previous)&31].width;
    }
    for(uint32_t i=1;i<b->points;i++){
        const int predicted=i<p->points,rising=b->levels[i]>b->levels[i-1];
        const uint32_t delta=predicted?(b->locations[i]-p->locations[i])&31:b->locations[i]-b->locations[i-1];
        if(stereo&&predicted&&rising){cost++;if(delta)cost+=direct_location(b,i);}
        else{
            const plus_gain_huff* table=predicted?(stereo?gain_location_stereo:(rising?gain_location_band_rise:gain_location_band_nonrise)):(rising?gain_location_within_rise:gain_location_within_nonrise);
            cost+=table[delta].width;
        }
    }return cost;
}
static int32_t location_cost(plus_given_gain_channel* c,const plus_given_gain_channel* ref,plus_given_gain_location_parameters* params,uint32_t mode){
    uint32_t cost=0;
    if(mode==3&&!c->channel_index){
        uint32_t minimum=31,maximum=0,points=0;
        for(uint32_t b=0;b<c->num_bands;b++)for(uint32_t i=0;i<c->bands[b].points;i++){
            uint32_t v=c->bands[b].locations[i]-i;if(v<minimum)minimum=v;if(v>maximum)maximum=v;points++;
        }
        if(!points)return PLUS_PACK_INVALID;
        params->base=minimum;params->width=(uint32_t)gain_location_range_width[maximum-minimum];
        return (int32_t)(7+points*params->width);
    }
    for(uint32_t b=0;b<c->num_bands;b++){
        const plus_given_gain_band* p=c->bands+b;
        if(mode==0||(mode==2&&!c->channel_index&&!b)){for(uint32_t i=0;i<p->points;i++)cost+=direct_location(p,i);}
        else if(mode==1&&!c->channel_index)cost+=within_locations(p);
        else if((mode==2&&!c->channel_index)||(mode==1&&c->channel_index))cost+=predicted_locations(p,c->channel_index?ref->bands+b:c->bands+b-1,c->channel_index!=0);
        else if(mode==2){
            params->explicit_values[b]=1;
            if(p->points){
                if(p->points<=ref->bands[b].points){
                    uint32_t different=0;cost++;
                    for(uint32_t i=0;i<p->points;i++)if(p->locations[i]!=ref->bands[b].locations[i])different=1;
                    params->explicit_values[b]=different;
                    if(!different)continue;
                }
                cost+=within_locations(p);
            }
        }else{
            params->extra_values[b]=0;
            for(uint32_t i=0;i<p->points;i++){
                if(i<ref->bands[b].points){if(p->locations[i]!=ref->bands[b].locations[i])return 0x4000;}
                else cost+=direct_location(p,i);
            }
            params->extra_values[b]=1;
        }
    }return (int32_t)cost;
}
int32_t plus_gain_mode_candidate(plus_given_gain_channel* c,const plus_given_gain_channel* ref,
    plus_given_gain_location_parameters* params,plus_gain_cost_component component,uint32_t mode){
    if(!valid(c)||mode>3||(c->channel_index&&!ref)||(component==PLUS_GAIN_COST_LOCATIONS&&!params))return PLUS_PACK_INVALID;
    if(component==PLUS_GAIN_COST_COUNTS)return count_cost(c,ref,mode);
    if(component==PLUS_GAIN_COST_LEVELS)return level_cost(c,ref,mode);
    if(component==PLUS_GAIN_COST_LOCATIONS)return location_cost(c,ref,params,mode);
    return PLUS_PACK_INVALID;
}
int32_t plus_gain_choose_component(plus_given_gain_channel* c,const plus_given_gain_channel* ref,
    plus_given_gain_location_parameters* params,plus_gain_cost_component component){
    int32_t costs[4];
    for(uint32_t mode=0;mode<4;mode++){costs[mode]=plus_gain_mode_candidate(c,ref,params,component,mode);if(costs[mode]<0)return costs[mode];}
    uint32_t best=0;
    for(uint32_t mode=1;mode<4;mode++)if(costs[mode]<costs[best])best=mode;
    if(component==PLUS_GAIN_COST_COUNTS)c->count_mode=best;
    else if(component==PLUS_GAIN_COST_LEVELS)c->level_mode=best;
    else c->location_mode=best;
    return costs[best];
}
int32_t plus_gain_choose_modes(plus_given_gain_channel* channels,plus_given_gain_location_parameters* params,int32_t channel_count,int32_t choose){
    if(channel_count>2||(channel_count>0&&(!channels||!params)))return PLUS_PACK_INVALID;
    uint32_t cost=channel_count>0?(uint32_t)channel_count:0;
    for(int32_t i=0;i<channel_count;i++)if(channels[i].present)cost+=11+(channels[i].repeat_flag?4:0);
    for(int32_t i=0;i<channel_count;i++){
        plus_given_gain_channel* c=channels+i;if(!c->present)continue;
        const plus_given_gain_channel* ref=(c->channel_index&&channel_count>1)?channels:0;
        if(!choose)c->count_mode=c->level_mode=c->location_mode=0;
        for(int32_t component=PLUS_GAIN_COST_COUNTS;component<=PLUS_GAIN_COST_LOCATIONS;component++){
            int32_t value=choose?plus_gain_choose_component(c,ref,params+i,(plus_gain_cost_component)component):plus_gain_mode_candidate(c,ref,params+i,(plus_gain_cost_component)component,0);
            if(value<0)return value;cost+=(uint32_t)value;
        }
    }
    return (int32_t)cost;
}
