#include "independent_plus_code_tables.h"
typedef struct plus_code_huff {uint16_t code;uint8_t width;} plus_code_huff;
#include "code_table_huffman.inc"
static int invalid(plus_bit_writer* w){if(w)w->error=PLUS_PACK_INVALID;return PLUS_PACK_INVALID;}
static int token(plus_bit_writer* w,const plus_code_huff* t,uint32_t index){return plus_pack_bits(w,t[index].code,t[index].width);}
int32_t plus_pack_code_table_values(plus_bit_writer* w,uint32_t full_table_flag,
    int32_t active_units,const plus_given_code_table_channel* c,const plus_given_code_table_channel* reference){
    if(!w||!c||full_table_flag>1||active_units>32)return invalid(w);
    if(w->error)return w->error;
    const uint32_t mode=c->mode&3,right=c->channel_index&1;
    /* The left-channel mode3 entry is the captured one-byte RET, including
     * omission of the optional-range flag. Preserve this exact native path.
     */
    if(mode==3&&!right)return (int32_t)w->bit_position;
    if(mode==3&&!reference)return invalid(w);
    if(c->partial_flag&&c->partial_count>31)return invalid(w);
    const int32_t count=c->partial_flag?(int32_t)c->partial_count:active_units;
    const uint32_t width=code_table_direct_widths[full_table_flag],mask=(1u<<width)-1u;
    for(int32_t q=0;q<count;q++)if(c->selectors[q]==1&&c->indices[q]>mask)return invalid(w);
    if(plus_pack_bits(w,c->partial_flag,1))return w->error;
    if(c->partial_flag&&plus_pack_bits(w,c->partial_count,5))return w->error;
    uint32_t previous=0;
    const plus_code_huff* absolute=full_table_flag?code_table_full_absolute:code_table_short;
    const plus_code_huff* delta=full_table_flag?code_table_full_delta:code_table_short;
    const plus_code_huff* stereo=full_table_flag?code_table_full_stereo_delta:code_table_short;
    for(int32_t q=0;q<count;q++){
        const uint32_t selector=c->selectors[q],value=c->indices[q];
        if(selector==2){if(plus_pack_bits(w,value,1))return w->error;}
        else if(selector==1){
            if(mode==0){if(plus_pack_bits(w,value,width))return w->error;}
            else if(mode==1||(mode==2&&q==0)){if(token(w,absolute,value))return w->error;}
            else if(mode==2){if(token(w,delta,(value-previous)&mask))return w->error;}
            else if(token(w,stereo,(value-reference->indices[q])&mask))return w->error;
            if(mode==2)previous=value;
        }
    }
    return (int32_t)w->bit_position;
}
int32_t plus_pack_code_tables(plus_bit_writer* w,uint32_t full_table_flag,
    int32_t active_units,const plus_given_code_table_channel* channels,int32_t channel_count){
    if(!w)return PLUS_PACK_INVALID;
    if(active_units<=0)return w->error?w->error:(int32_t)w->bit_position;
    if(full_table_flag>1||active_units>32||channel_count>2||(channel_count>0&&!channels))return invalid(w);
    if(plus_pack_bits(w,full_table_flag,1))return w->error;
    for(int32_t ch=0;ch<channel_count;ch++){
        const plus_given_code_table_channel* c=channels+ch;
        if(plus_pack_bits(w,c->table_flag,1)||plus_pack_bits(w,c->mode,2))return w->error;
        if(plus_pack_code_table_values(w,full_table_flag,active_units,c,(c->channel_index&1)?channels:NULL)<0)return w->error;
    }
    return (int32_t)w->bit_position;
}
