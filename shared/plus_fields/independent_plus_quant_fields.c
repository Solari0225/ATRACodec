#include "independent_plus_quant_fields.h"
typedef struct plus_quant_huff {uint16_t code;uint8_t width;} plus_quant_huff;
#include "quant_huffman.inc"
static int invalid(plus_bit_writer* w){if(w)w->error=PLUS_PACK_INVALID;return PLUS_PACK_INVALID;}
static int token(plus_bit_writer* w,const plus_quant_huff* t,uint32_t i){return plus_pack_bits(w,t[i].code,t[i].width);}
int32_t plus_word_length_symbol_width(uint32_t table,uint32_t symbol){return table<4&&symbol<8?wl_huffman[table][symbol].width:PLUS_PACK_INVALID;}
int32_t plus_scale_factor_symbol_width(uint32_t short_alphabet,uint32_t table,uint32_t symbol){
    if(table>=4||symbol>=(short_alphabet?16u:64u))return PLUS_PACK_INVALID;
    return short_alphabet?sf16_huffman[table][symbol].width:sf64_huffman[table][symbol].width;
}
static int wl_range(plus_bit_writer* w,const plus_given_word_length* c){
    if(c->range_mode){
        if(plus_pack_bits(w,c->packed_count,5))return w->error;
        if(c->range_mode==3&&plus_pack_bits(w,c->fill_extra-(c->channel_index==0?1u:3u),2))return w->error;
    }return 0;
}
static int wl_tail(plus_bit_writer* w,const plus_given_word_length_global* g,const plus_given_word_length* c){
    for(uint32_t i=c->packed_count;i<(uint32_t)g->total_units;i++)if(plus_pack_bits(w,c->values[i],1))return w->error;
    return 0;
}
int32_t plus_pack_word_length_values(plus_bit_writer* w,const plus_given_word_length_global* g,
    const plus_given_word_length* c,const plus_given_word_length* ref){
    if(!w||!g||!c||g->total_units<0||g->total_units>32)return invalid(w);
    if(w->error)return w->error;
    const uint32_t mode=c->mode&3,right=c->channel_index&1,n=c->packed_count;
    if(mode==0){for(int32_t i=0;i<g->total_units;i++)if(plus_pack_bits(w,c->raw[i],3))return w->error;return (int32_t)w->bit_position;}
    if(n>32)return invalid(w);
    if(n&&mode==1&&!right&&c->prefix_count>32)return invalid(w);
    if(n&&!(mode==1&&!right)&&c->huffman_id>3)return invalid(w);
    if(n&&mode==2&&!right&&c->group_enable&&(g->group_count<0||g->group_count>16))return invalid(w);
    if(right&&(mode==1||mode==2)&&!ref)return invalid(w);
    if(((mode==1&&!right)||mode==3)&&plus_pack_bits(w,c->weight_id,2))return w->error;
    if(plus_pack_bits(w,c->range_mode,2)||wl_range(w,c))return w->error;
    if(n){
        if(mode==1&&!right){
            if(plus_pack_bits(w,c->prefix_count,5)||plus_pack_bits(w,c->suffix_width,2)||plus_pack_bits(w,c->suffix_base,3))return w->error;
            for(uint32_t i=0;i<c->prefix_count;i++)if(plus_pack_bits(w,c->values[i],3))return w->error;
            if(c->suffix_width)for(uint32_t i=c->prefix_count;i<n;i++)if(plus_pack_bits(w,c->values[i]-c->suffix_base,c->suffix_width))return w->error;
        }else if(mode==2&&!right){
            if(plus_pack_bits(w,c->group_enable,1)||plus_pack_bits(w,c->huffman_id,1)||plus_pack_bits(w,c->mode2_param3,3)||plus_pack_bits(w,c->mode2_param4,4))return w->error;
            uint32_t start=0;
            if(c->group_enable){
                for(int32_t group=0;group<g->group_count;group++){
                    if(plus_pack_bits(w,g->group_flags[group],1))return w->error;
                    if(!g->group_flags[group])for(uint32_t i=(uint32_t)group*2;i<(uint32_t)group*2+2;i++){
                        if(c->residual[i]>7)return invalid(w);
                        if(token(w,wl_huffman[c->huffman_id],c->residual[i]))return w->error;
                    }
                }start=(uint32_t)g->group_count*2;
            }
            for(uint32_t i=start;i<n;i++){
                if(c->residual[i]>7)return invalid(w);
                if(token(w,wl_huffman[c->huffman_id],c->residual[i]))return w->error;
            }
        }else{
            if(plus_pack_bits(w,c->huffman_id,2))return w->error;
            if(mode==3){
                if(plus_pack_bits(w,c->values[0],3))return w->error;
                for(uint32_t i=1;i<n;i++)if(token(w,wl_huffman[c->huffman_id],(c->values[i]-c->values[i-1])&7))return w->error;
            }else if(mode==1){
                for(uint32_t i=0;i<n;i++)if(token(w,wl_huffman[c->huffman_id],(c->raw[i]-ref->raw[i])&7))return w->error;
            }else{
                if(token(w,wl_huffman[c->huffman_id],(c->raw[0]-ref->raw[0])&7))return w->error;
                for(uint32_t i=1;i<n;i++)if(token(w,wl_huffman[c->huffman_id],((c->raw[i]-ref->raw[i])-(c->raw[i-1]-ref->raw[i-1]))&7))return w->error;
            }
        }
    }
    if(c->range_mode==2&&((mode==3&&c->channel_index==1)||(right&&(mode==1||mode==2))))if(wl_tail(w,g,c))return w->error;
    return (int32_t)w->bit_position;
}
int32_t plus_pack_word_lengths(plus_bit_writer* w,const plus_given_word_length_global* g,
    const plus_given_word_length* channels,int32_t channel_count){
    if(!w||!g||channel_count>2||(channel_count>0&&!channels))return invalid(w);
    for(int32_t ch=0;ch<channel_count;ch++){
        if(plus_pack_bits(w,channels[ch].mode,2))return w->error;
        if(plus_pack_word_length_values(w,g,channels+ch,(channels[ch].channel_index&1)?channels:NULL)<0)return w->error;
    }return w->error?w->error:(int32_t)w->bit_position;
}
int32_t plus_pack_scale_factor_values(plus_bit_writer* w,int32_t active_units,
    const plus_given_scale_factor* c,const plus_given_scale_factor* ref){
    if(!w||!c||active_units<0||active_units>32)return invalid(w);
    if(w->error)return w->error;
    const uint32_t mode=c->mode&3,right=c->channel_index&1,n=(uint32_t)active_units;
    if(mode==0){for(uint32_t i=0;i<n;i++)if(plus_pack_bits(w,c->raw[i],6))return w->error;return (int32_t)w->bit_position;}
    if(mode==3&&right)return (int32_t)w->bit_position;
    if(right&&!ref)return invalid(w);
    if((!right&&(mode==1||mode==3)&&c->weight_id>3)||(!right&&mode==1&&c->prefix_count>32))return invalid(w);
    if(((right&&mode==2)||(right&&mode==1&&n)||(!right&&mode==2&&n)||(!right&&mode==3&&n>1))&&c->huffman_id>3)return invalid(w);
    if(right){
        if(plus_pack_bits(w,c->huffman_id,2))return w->error;
        if(mode==1){for(uint32_t i=0;i<n;i++)if(token(w,sf64_huffman[c->huffman_id],(c->raw[i]-ref->raw[i])&63))return w->error;}
        else{
            if(token(w,sf64_huffman[c->huffman_id],(c->raw[0]-ref->raw[0])&63))return w->error;
            for(uint32_t i=1;i<n;i++)if(token(w,sf64_huffman[c->huffman_id],((c->raw[i]-ref->raw[i])-(c->raw[i-1]-ref->raw[i-1]))&63))return w->error;
        }
    }else if(mode==1){
        if(plus_pack_bits(w,c->weight_id,2))return w->error;
        const uint32_t* values=c->prepared[c->weight_id];
        if(c->weight_id!=3){
            if(plus_pack_bits(w,c->prefix_count,5)||plus_pack_bits(w,c->suffix_width,3)||plus_pack_bits(w,c->suffix_base,6))return w->error;
            for(uint32_t i=0;i<c->prefix_count;i++)if(plus_pack_bits(w,values[i],6))return w->error;
        }else{
            if(plus_pack_bits(w,c->offset_a,6)||plus_pack_bits(w,c->offset_b,6)||plus_pack_bits(w,c->prefix_count,5)||plus_pack_bits(w,c->suffix_width,2)||plus_pack_bits(w,c->suffix_base+7,4))return w->error;
            for(uint32_t i=0;i<c->prefix_count;i++)if(plus_pack_bits(w,values[i]+7,4))return w->error;
        }
        if(c->suffix_width)for(uint32_t i=c->prefix_count;i<n;i++)if(plus_pack_bits(w,values[i]-c->suffix_base,c->suffix_width))return w->error;
    }else if(mode==2){
        if(plus_pack_bits(w,c->huffman_id,2)||plus_pack_bits(w,c->offset_a,6)||plus_pack_bits(w,c->offset_b,6))return w->error;
        for(uint32_t i=0;i<n;i++)if(token(w,sf16_huffman[c->huffman_id],c->prepared[3][i]&15))return w->error;
    }else{
        if(plus_pack_bits(w,c->weight_id,2)||plus_pack_bits(w,c->huffman_id,2))return w->error;
        const uint32_t* values=c->prepared[c->weight_id];
        if(c->weight_id!=3){
            if(plus_pack_bits(w,values[0],6))return w->error;
            for(uint32_t i=1;i<n;i++)if(token(w,sf64_huffman[c->huffman_id],(values[i]-values[i-1])&63))return w->error;
        }else{
            if(plus_pack_bits(w,c->offset_a,6)||plus_pack_bits(w,c->offset_b,6)||plus_pack_bits(w,values[0]+8,4))return w->error;
            for(uint32_t i=1;i<n;i++)if(token(w,sf16_huffman[c->huffman_id],(values[i]-values[i-1])&15))return w->error;
        }
    }
    return (int32_t)w->bit_position;
}
int32_t plus_pack_scale_factors(plus_bit_writer* w,int32_t active_units,
    const plus_given_scale_factor* channels,int32_t channel_count){
    if(!w)return PLUS_PACK_INVALID;
    if(active_units<=0)return w->error?w->error:(int32_t)w->bit_position;
    if(channel_count>2||(channel_count>0&&!channels))return invalid(w);
    for(int32_t ch=0;ch<channel_count;ch++){
        if(plus_pack_bits(w,channels[ch].mode,2))return w->error;
        if(plus_pack_scale_factor_values(w,active_units,channels+ch,(channels[ch].channel_index&1)?channels:NULL)<0)return w->error;
    }return w->error?w->error:(int32_t)w->bit_position;
}
