#include "independent_plus_gain.h"
typedef struct plus_gain_huff {uint16_t code;uint8_t width;} plus_gain_huff;
#include "gain_huffman.inc"
#include "gain_location_huffman.inc"
static int invalid(plus_bit_writer* w){if(w)w->error=PLUS_PACK_INVALID;return PLUS_PACK_INVALID;}
static int valid(const plus_given_gain_channel* c,int check_levels){
    if(!c||c->num_bands<1||c->num_bands>16)return 0;
    for(uint32_t b=0;b<c->num_bands;b++){
        if(c->bands[b].points>7)return 0;
        if(check_levels)for(uint32_t p=0;p<c->bands[b].points;p++)if(c->bands[b].levels[p]>15)return 0;
    }return 1;
}
static int token(plus_bit_writer* w,const plus_gain_huff* table,uint32_t index){return plus_pack_bits(w,table[index].code,table[index].width);}
static int within_band_levels(plus_bit_writer* w,const plus_given_gain_band* b){
    if(b->points){
        if(token(w,gain_level_absolute,b->levels[0]))return w->error;
        for(uint32_t p=1;p<b->points;p++)if(token(w,gain_level_within_band_delta,(b->levels[p]-b->levels[p-1])&15))return w->error;
    }return 0;
}
int32_t plus_pack_gain_counts(plus_bit_writer* w,const plus_given_gain_channel* c,const plus_given_gain_channel* ref){
    if(!w||!valid(c,0))return invalid(w);
    const uint32_t mode=c->count_mode&3,right=c->channel_index&1;
    if(mode==2&&right&&!ref)return invalid(w);
    if(mode==3&&!right&&c->count_width>3)return invalid(w);
    if(plus_pack_bits(w,c->count_mode,2))return w->error;
    if(mode==3){
        if(!right){
            if(plus_pack_bits(w,c->count_width,2)||plus_pack_bits(w,c->count_base,3))return w->error;
            if(c->count_width)for(uint32_t b=0;b<c->num_bands;b++)if(plus_pack_bits(w,c->bands[b].points-c->count_base,c->count_width))return w->error;
        }
    }else for(uint32_t b=0;b<c->num_bands;b++){
        const uint32_t points=c->bands[b].points;
        if(mode==0){if(plus_pack_bits(w,points,3))return w->error;}
        else if(mode==1||(mode==2&&!right&&b==0)){if(token(w,gain_count_absolute,points))return w->error;}
        else{
            const uint32_t prediction=right?ref->bands[b].points:c->bands[b-1].points;
            if(token(w,gain_count_delta,(points-prediction)&7))return w->error;
        }
    }
    return (int32_t)w->bit_position;
}
int32_t plus_pack_gain_levels(plus_bit_writer* w,const plus_given_gain_channel* c,const plus_given_gain_channel* ref){
    if(!w||!valid(c,1))return invalid(w);
    const uint32_t mode=c->level_mode&3,right=c->channel_index&1;
    if(mode==1&&right&&!ref)return invalid(w);
    if(mode==3&&!right&&c->level_width>3)return invalid(w);
    if(plus_pack_bits(w,c->level_mode,2))return w->error;
    if(mode==3){
        if(!right){
            if(plus_pack_bits(w,c->level_width,2)||plus_pack_bits(w,c->level_base,4))return w->error;
            if(c->level_width)for(uint32_t b=0;b<c->num_bands;b++)for(uint32_t p=0;p<c->bands[b].points;p++)
                if(plus_pack_bits(w,c->bands[b].levels[p]-c->level_base,c->level_width))return w->error;
        }
    }else for(uint32_t b=0;b<c->num_bands;b++){
        const plus_given_gain_band* band=&c->bands[b];
        if(mode==0){for(uint32_t p=0;p<band->points;p++)if(plus_pack_bits(w,band->levels[p],4))return w->error;}
        else if(mode==1){
            if(!right){if(within_band_levels(w,band))return w->error;}
            else for(uint32_t p=0;p<band->points;p++){
                const uint32_t pred=p<ref->bands[b].points?ref->bands[b].levels[p]:7;
                if(token(w,gain_level_stereo_delta,(band->levels[p]-pred)&15))return w->error;
            }
        }else if(right){
            if(band->points){
                if(plus_pack_bits(w,c->level_explicit[b],1))return w->error;
                if(c->level_explicit[b]&&within_band_levels(w,band))return w->error;
            }
        }else if(b==0){if(within_band_levels(w,band))return w->error;}
        else for(uint32_t p=0;p<band->points;p++){
            const plus_given_gain_band* previous=&c->bands[b-1];
            const uint32_t pred=p<previous->points?previous->levels[p]:7;
            if(token(w,gain_level_band_delta,(band->levels[p]-pred)&15))return w->error;
        }
    }
    return (int32_t)w->bit_position;
}
int32_t plus_pack_gain_locations_direct(plus_bit_writer* w,const plus_given_gain_channel* c){
    if(!w||!valid(c,0)||c->location_mode!=0)return invalid(w);
    if(plus_pack_bits(w,0,2))return w->error;
    for(uint32_t b=0;b<c->num_bands;b++)for(uint32_t p=0;p<c->bands[b].points;p++){
        const uint32_t value=c->bands[b].locations[p];
        if(p==0){if(plus_pack_bits(w,value,5))return w->error;continue;}
        const uint32_t previous=c->bands[b].locations[p-1];
        if(previous<15){if(plus_pack_bits(w,value,5))return w->error;}
        else if(previous<23){if(plus_pack_bits(w,value-previous-1,4))return w->error;}
        else if(previous<27){if(plus_pack_bits(w,value-previous-1,3))return w->error;}
        else if(previous<29){if(plus_pack_bits(w,value-previous-1,2))return w->error;}
        else if(previous==29){if(plus_pack_bits(w,value-30,1))return w->error;}
    }
    return (int32_t)w->bit_position;
}
int32_t plus_pack_gain_direct_locations(plus_bit_writer* w,const plus_given_gain_channel* channels,int32_t channel_count){
    if(!w||(channel_count>0&&!channels))return invalid(w);
    for(int32_t ch=0;ch<channel_count;ch++)if(channels[ch].present&&(!valid(channels+ch,1)||channels[ch].location_mode!=0))return invalid(w);
    for(int32_t ch=0;ch<channel_count;ch++){
        const plus_given_gain_channel* c=channels+ch;
        if(plus_pack_bits(w,c->present,1))return w->error;
        if(!c->present)continue;
        if(plus_pack_bits(w,c->num_bands-1,4)||plus_pack_bits(w,c->repeat_flag,1))return w->error;
        if(c->repeat_flag&&plus_pack_bits(w,c->repeat_bands-1,4))return w->error;
        const plus_given_gain_channel* ref=(c->channel_index&1)?channels:NULL;
        if(plus_pack_gain_counts(w,c,ref)<0||plus_pack_gain_levels(w,c,ref)<0||plus_pack_gain_locations_direct(w,c)<0)return w->error;
    }
    return (int32_t)w->bit_position;
}
static int direct_location_point(plus_bit_writer* w,const plus_given_gain_band* b,uint32_t p){
    const uint32_t v=b->locations[p];
    if(p==0||b->locations[p-1]<15)return plus_pack_bits(w,v,5);
    const uint32_t previous=b->locations[p-1];
    if(previous<23)return plus_pack_bits(w,v-previous-1,4);
    if(previous<27)return plus_pack_bits(w,v-previous-1,3);
    if(previous<29)return plus_pack_bits(w,v-previous-1,2);
    if(previous==29)return plus_pack_bits(w,v-30,1);
    return 0;
}
static int within_band_locations(plus_bit_writer* w,const plus_given_gain_band* b){
    if(b->points&&plus_pack_bits(w,b->locations[0],5))return w->error;
    for(uint32_t p=1;p<b->points;p++){
        const plus_gain_huff* t=b->levels[p]>b->levels[p-1]?gain_location_within_rise:gain_location_within_nonrise;
        if(token(w,t,b->locations[p]-b->locations[p-1]))return w->error;
    }
    return 0;
}
int32_t plus_pack_gain_locations(plus_bit_writer* w,const plus_given_gain_channel* c,
    const plus_given_gain_channel* ref,const plus_given_gain_location_parameters* parameters){
    if(!w||!valid(c,1)||c->location_mode>3)return invalid(w);
    const uint32_t mode=c->location_mode,right=c->channel_index&1;
    if(mode==0)return plus_pack_gain_locations_direct(w,c);
    if(right&&!ref)return invalid(w);
    if((mode==3||(right&&mode==2))&&!parameters)return invalid(w);
    if(mode==3&&!right&&(parameters->width<1||parameters->width>4))return invalid(w);
    for(uint32_t b=0;b<c->num_bands;b++)for(uint32_t p=0;p<c->bands[b].points;p++)
        if(c->bands[b].locations[p]>31||(p&&c->bands[b].locations[p]<=c->bands[b].locations[p-1]))return invalid(w);
    if(plus_pack_bits(w,c->location_mode,2))return w->error;
    if(mode==3&&!right){
        if(plus_pack_bits(w,parameters->width-1,2)||plus_pack_bits(w,parameters->base,5))return w->error;
        for(uint32_t b=0;b<c->num_bands;b++)for(uint32_t p=0;p<c->bands[b].points;p++)
            if(plus_pack_bits(w,c->bands[b].locations[p]-parameters->base-p,parameters->width))return w->error;
        return (int32_t)w->bit_position;
    }
    for(uint32_t b=0;b<c->num_bands;b++){
        const plus_given_gain_band* band=&c->bands[b];
        if(mode==1&&!right){if(within_band_locations(w,band))return w->error;}
        else if(mode==2&&!right){
            if(b==0){for(uint32_t p=0;p<band->points;p++)if(direct_location_point(w,band,p))return w->error;continue;}
            const plus_given_gain_band* previous=&c->bands[b-1];
            if(band->points){
                const uint32_t predicted=previous->points?previous->locations[0]:0;
                if(token(w,gain_location_band_nonrise,(band->locations[0]-predicted)&31))return w->error;
            }
            for(uint32_t p=1;p<band->points;p++){
                const int predicted=p<previous->points,rising=band->levels[p]>band->levels[p-1];
                const uint32_t delta=predicted?(band->locations[p]-previous->locations[p])&31:band->locations[p]-band->locations[p-1];
                const plus_gain_huff* t=predicted?(rising?gain_location_band_rise:gain_location_band_nonrise):(rising?gain_location_within_rise:gain_location_within_nonrise);
                if(token(w,t,delta))return w->error;
            }
        }else if(mode==1&&right){
            const plus_given_gain_band* reference=&ref->bands[b];
            if(band->points){
                const uint32_t predicted=reference->points?reference->locations[0]:0;
                if(token(w,gain_location_stereo,(band->locations[0]-predicted)&31))return w->error;
            }
            for(uint32_t p=1;p<band->points;p++){
                const int predicted=p<reference->points,rising=band->levels[p]>band->levels[p-1];
                const uint32_t delta=predicted?(band->locations[p]-reference->locations[p])&31:band->locations[p]-band->locations[p-1];
                if(predicted&&rising){
                    if(plus_pack_bits(w,delta!=0,1))return w->error;
                    if(delta&&direct_location_point(w,band,p))return w->error;
                }else{
                    const plus_gain_huff* t=predicted?gain_location_stereo:(rising?gain_location_within_rise:gain_location_within_nonrise);
                    if(token(w,t,delta))return w->error;
                }
            }
        }else if(mode==2&&right){
            if(band->points){
                if(band->points<=ref->bands[b].points){
                    if(plus_pack_bits(w,parameters->explicit_values[b],1))return w->error;
                    if(!parameters->explicit_values[b])continue;
                }
                if(within_band_locations(w,band))return w->error;
            }
        }else if(mode==3&&right){
            if(parameters->extra_values[b]&&ref->bands[b].points<band->points)
                for(uint32_t p=ref->bands[b].points;p<band->points;p++)if(direct_location_point(w,band,p))return w->error;
        }
    }
    return (int32_t)w->bit_position;
}
int32_t plus_pack_gain(plus_bit_writer* w,const plus_given_gain_channel* channels,
    const plus_given_gain_location_parameters* parameters,int32_t channel_count){
    if(!w||channel_count>2||(channel_count>0&&!channels))return invalid(w);
    for(int32_t ch=0;ch<channel_count;ch++)if(channels[ch].present&&(!valid(channels+ch,1)||channels[ch].location_mode>3))return invalid(w);
    for(int32_t ch=0;ch<channel_count;ch++){
        const plus_given_gain_channel* c=channels+ch;
        if(plus_pack_bits(w,c->present,1))return w->error;
        if(!c->present)continue;
        if(plus_pack_bits(w,c->num_bands-1,4)||plus_pack_bits(w,c->repeat_flag,1))return w->error;
        if(c->repeat_flag&&plus_pack_bits(w,c->repeat_bands-1,4))return w->error;
        const plus_given_gain_channel* ref=(c->channel_index&1)?channels:NULL;
        if(plus_pack_gain_counts(w,c,ref)<0||plus_pack_gain_levels(w,c,ref)<0||plus_pack_gain_locations(w,c,ref,parameters?parameters+ch:NULL)<0)return w->error;
    }
    return (int32_t)w->bit_position;
}
