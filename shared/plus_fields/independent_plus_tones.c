#include "independent_plus_tones.h"
typedef struct tone_huff {uint16_t code;uint8_t width;} tone_huff;
#include "tone_math.inc"
static int invalid(plus_bit_writer* w){if(w)w->error=PLUS_PACK_INVALID;return PLUS_PACK_INVALID;}
static int huff(plus_bit_writer* w,uint32_t table,uint32_t symbol){
    if(table>=10||symbol>=tone_book_sizes[table])return invalid(w);
    const tone_huff* h=tone_codes+tone_book_starts[table]+symbol;
    return plus_pack_bits(w,h->code,h->width);
}
static int ascending(plus_bit_writer* w,const plus_given_tone_band* b){
    for(uint32_t i=0;i<b->count;i++){
        uint32_t width=10,base=0;
        if(i){
            const int32_t previous=(int32_t)b->tones[i-1].frequency;
            while(width>1&&previous>=(int32_t)(1024-(1u<<(width-1))))width--;
            base=1024-(1u<<width);
        }
        if(plus_pack_bits(w,b->tones[i].frequency-base,width))return w->error;
    }return 0;
}
static int descending(plus_bit_writer* w,const plus_given_tone_band* b){
    for(uint32_t i=b->count;i>0;i--){
        uint32_t width=10;
        if(i<b->count){
            const int32_t next=(int32_t)b->tones[i].frequency;width=1;
            while(width<10&&next>=(int32_t)(1u<<width))width++;
        }
        if(plus_pack_bits(w,b->tones[i-1].frequency,width))return w->error;
    }return 0;
}
int32_t plus_pack_tone_substage(plus_bit_writer* w,const plus_given_tone_global* g,
    const plus_given_tone_channel* c,const plus_given_tone_channel* ref,plus_tone_substage stage){
    if(!w||!g||!c||g->band_count<0||g->band_count>16||c->channel_index>1)return invalid(w);
    if(w->error)return w->error;
    const uint32_t right=c->channel_index;
    uint32_t mode;
    if(stage==PLUS_TONE_ENVELOPE){
        mode=c->envelope_mode&1;
        if(right&&plus_pack_bits(w,c->envelope_mode,1))return w->error;
    }else if(stage==PLUS_TONE_COUNT){
        mode=c->count_mode&3;
        if(plus_pack_bits(w,c->count_mode,tone_count_mode_width[right]))return w->error;
    }else if(stage==PLUS_TONE_FREQUENCY){
        mode=c->frequency_mode&1;
        if(right&&plus_pack_bits(w,c->frequency_mode,1))return w->error;
    }else if(stage==PLUS_TONE_AMPLITUDE){
        mode=c->amplitude_mode&3;
        if(plus_pack_bits(w,c->amplitude_mode,tone_amplitude_mode_width[right]))return w->error;
    }else if(stage==PLUS_TONE_FINE_AMPLITUDE){
        mode=c->fine_amplitude_mode&3;
        if(plus_pack_bits(w,c->fine_amplitude_mode,tone_fine_mode_width[right]))return w->error;
    }else if(stage==PLUS_TONE_PHASE)mode=0;
    else return invalid(w);
    if((stage==PLUS_TONE_ENVELOPE&&mode==1)||
       ((stage==PLUS_TONE_COUNT||stage==PLUS_TONE_AMPLITUDE||stage==PLUS_TONE_FINE_AMPLITUDE)&&mode==3))return (int32_t)w->bit_position;
    if((stage==PLUS_TONE_COUNT&&mode==2)||(stage==PLUS_TONE_FREQUENCY&&mode==1)||
       (stage==PLUS_TONE_AMPLITUDE&&mode==2)||(stage==PLUS_TONE_FINE_AMPLITUDE&&mode==2))if(!ref)return invalid(w);
    uint32_t flat=0;
    for(int32_t band=0;band<g->band_count;band++){
        if(!c->selected[band])continue;
        const plus_given_tone_band* b=c->bands+band;
        const plus_given_tone_band* p=ref?ref->bands+band:0;
        if(b->count>15)return invalid(w);
        if(stage==PLUS_TONE_ENVELOPE){
            if(plus_pack_bits(w,b->envelope[0],1))return w->error;
            if(b->envelope[0]&&plus_pack_bits(w,b->envelope[2],5))return w->error;
            if(plus_pack_bits(w,b->envelope[1],1))return w->error;
            if(b->envelope[1]&&plus_pack_bits(w,b->envelope[3],5))return w->error;
        }else if(stage==PLUS_TONE_COUNT){
            if(mode==0){if(plus_pack_bits(w,b->count,4))return w->error;}
            else if(mode==1){if(huff(w,1,b->count))return w->error;}
            else if(huff(w,2,(b->count-p->count)&7))return w->error;
        }else if(stage==PLUS_TONE_FREQUENCY){
            if(mode==0){
                const uint32_t direction=c->frequency_direction[band];
                if(b->count>1&&plus_pack_bits(w,direction,1))return w->error;
                if(direction==0){if(ascending(w,b))return w->error;}
                else if(direction==1&&descending(w,b))return w->error;
            }else for(uint32_t i=0;i<b->count;i++){
                const uint32_t previous=p->count?p->tones[i<p->count?i:p->count-1].frequency:0;
                if(huff(w,3,(b->tones[i].frequency-previous)&255))return w->error;
            }
        }else if(stage==PLUS_TONE_AMPLITUDE){
            const uint32_t count=g->amplitude_mode?b->count:(b->count!=0);
            for(uint32_t i=0;i<count;i++){
                const uint32_t amplitude=b->tones[i].amplitude;
                if(mode==0){if(plus_pack_bits(w,amplitude,6))return w->error;}
                else if(mode==1){if(huff(w,g->amplitude_mode?5:4,amplitude-(g->amplitude_mode?20u:24u)))return w->error;}
                else{
                    uint32_t previous;
                    if(!g->amplitude_mode)previous=p->count?p->tones[0].amplitude:44;
                    else{
                        if(flat+i>=256)return invalid(w);
                        const int32_t index=g->stereo_map[flat+i];
                        if(index>=16)return invalid(w);
                        previous=index>=0?p->tones[index].amplitude:34;
                    }
                    if(huff(w,6,(amplitude-previous)&31))return w->error;
                }
            }
            flat+=b->count;
        }else if(stage==PLUS_TONE_FINE_AMPLITUDE){
            for(uint32_t i=0;i<b->count;i++){
                const uint32_t value=b->tones[i].fine_amplitude;
                if(mode==0){if(plus_pack_bits(w,value,4))return w->error;}
                else if(mode==1){if(huff(w,b->count==1?7:8,value))return w->error;}
                else{
                    if(flat+i>=256)return invalid(w);
                    const int32_t index=g->stereo_map[flat+i];
                    if(index>=16)return invalid(w);
                    const uint32_t previous=index>=0?p->tones[index].fine_amplitude:12;
                    if(huff(w,9,(value-previous)&7))return w->error;
                }
            }
            flat+=b->count;
        }else for(uint32_t i=0;i<b->count;i++)if(plus_pack_bits(w,b->tones[i].phase,5))return w->error;
    }
    return (int32_t)w->bit_position;
}
int32_t plus_pack_tones(plus_bit_writer* w,const plus_given_tone_global* g,
    const plus_given_tone_channel* channels,int32_t channel_count){
    if(!w||!g||channel_count>2||(channel_count>0&&!channels))return invalid(w);
    if(plus_pack_bits(w,g->present,1))return w->error;
    if(!g->present)return (int32_t)w->bit_position;
    if(g->band_count<1||g->band_count>16)return invalid(w);
    if(plus_pack_bits(w,g->amplitude_mode,1)||huff(w,0,(uint32_t)g->band_count-1))return w->error;
    if(channel_count==2)for(uint32_t i=0;i<3;i++)if(plus_pack_flag_vector(w,g->stereo_flags+i,g->band_count)<0)return w->error;
    for(int32_t ch=0;ch<channel_count;ch++){
        const plus_given_tone_channel* c=channels+ch;
        const plus_given_tone_channel* ref=channel_count==2?channels+(1-ch):0;
        for(int32_t stage=PLUS_TONE_ENVELOPE;stage<=PLUS_TONE_PHASE;stage++){
            if(stage==PLUS_TONE_FINE_AMPLITUDE&&g->amplitude_mode)continue;
            if(plus_pack_tone_substage(w,g,c,ref,(plus_tone_substage)stage)<0)return w->error;
        }
    }
    return (int32_t)w->bit_position;
}
