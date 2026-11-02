#include "independent_plus_spectrum.h"
typedef struct spectrum_huff {uint16_t code;uint8_t width;} spectrum_huff;
#include "spectrum_math.inc"
static int invalid(plus_bit_writer* w){if(w)w->error=PLUS_PACK_INVALID;return PLUS_PACK_INVALID;}
const plus_spectrum_descriptor* plus_spectrum_book(uint32_t index){return index>0&&index<=112?spectrum_books+index:0;}
int32_t plus_spectrum_symbol_width(uint32_t index,uint32_t symbol){
    const plus_spectrum_descriptor* b=plus_spectrum_book(index);
    return b&&symbol<b->row_count?spectrum_codes[b->first_row+symbol].width:PLUS_PACK_INVALID;
}
int32_t plus_spectrum_make_symbols(const int16_t* input,uint32_t n,uint32_t index,
    int16_t* symbols,uint32_t* sign_counts,uint32_t* sign_values){
    const plus_spectrum_descriptor* b=plus_spectrum_book(index);
    if(!b||!input||!symbols||n>128||n%b->coefficient_group||
       (b->extra_signs&&(!sign_counts||!sign_values)))return PLUS_PACK_INVALID;
    const uint32_t group=b->coefficient_group;
    for(uint32_t i=0;i<n/group;i++){
        uint32_t packed=0,signs=0,count=0;
        for(uint32_t j=0;j<group;j++){
            const int32_t v=input[i*group+j];
            uint32_t magnitude=(uint32_t)v&b->coefficient_mask;
            if(b->extra_signs){
                magnitude=v<0?(uint32_t)(-v)&b->coefficient_mask:(uint32_t)v;
                if(v){signs=signs*2+(v<0);count++;}
            }
            packed=((packed<<b->coefficient_bits)|magnitude)&65535u;
        }
        symbols[i]=(int16_t)(uint16_t)packed;
        if(b->extra_signs){sign_counts[i]=count;sign_values[i]=signs;}
    }
    return (int32_t)(n/group);
}
static int emit_symbol(plus_bit_writer* w,const plus_spectrum_descriptor* b,
    uint32_t symbol,uint32_t count,uint32_t signs){
    if(symbol>=b->row_count)return invalid(w);
    const spectrum_huff* h=spectrum_codes+b->first_row+symbol;
    if(plus_pack_bits(w,h->code,h->width))return w->error;
    if(b->extra_signs&&count&&plus_pack_bits(w,signs,count))return w->error;
    return 0;
}
int32_t plus_pack_spectrum_unit(plus_bit_writer* w,const int16_t* input,uint32_t n,uint32_t index){
    const plus_spectrum_descriptor* b=plus_spectrum_book(index);
    if(!w||!b)return invalid(w);
    if(w->error)return w->error;
    int16_t symbols[128];uint32_t counts[128],signs[128];
    const int32_t count=plus_spectrum_make_symbols(input,n,index,symbols,counts,signs);
    if(count<0||count%b->symbol_group)return invalid(w);
    for(uint32_t i=0;i<(uint32_t)count;i+=b->symbol_group){
        uint32_t present=0;
        if(b->symbol_group>1){
            for(uint32_t j=0;j<b->symbol_group;j++)present|=(uint16_t)symbols[i+j];
            if(plus_pack_bits(w,present!=0,1))return w->error;
            if(!present)continue;
        }
        for(uint32_t j=0;j<b->symbol_group;j++){
            const uint32_t k=i+j;
            if(emit_symbol(w,b,(uint16_t)symbols[k],b->extra_signs?counts[k]:0,b->extra_signs?signs[k]:0))return w->error;
        }
    }
    return (int32_t)w->bit_position;
}
int32_t plus_pack_spectrum(plus_bit_writer* w,const plus_given_spectrum_global* g,
    const plus_given_spectrum_channel* channels,int32_t channel_count){
    if(!w||!g||g->active_units<0||g->active_units>32||g->subbands<0||g->subbands>16||
       channel_count>2||(channel_count>0&&!channels))return invalid(w);
    for(int32_t ch=0;ch<channel_count;ch++){
        const plus_given_spectrum_channel* c=channels+ch;
        for(int32_t unit=0;unit<g->active_units;unit++){
            const int32_t wl=(int32_t)c->word_lengths[unit];
            if(wl<=0)continue;
            uint32_t book=c->codebooks[unit];
            if(wl>7||c->table_bank>1)return invalid(w);
            if(!g->full_table){
                if(book>3)return invalid(w);
                book=spectrum_short_map[book+4*(7*c->table_bank+(uint32_t)wl)];
            }
            if(book>7)return invalid(w);
            const uint32_t index=7*(book+8*c->table_bank)+(uint32_t)wl;
            if(plus_pack_spectrum_unit(w,c->coefficients+spectrum_qu_start[unit],spectrum_qu_count[unit],index)<0)return w->error;
        }
        if(g->active_units>2){
            const uint32_t n=(uint32_t)spectrum_trailing_count[g->subbands]+1;
            if(n>16)return invalid(w);
            for(uint32_t i=0;i<n;i++)if(plus_pack_bits(w,c->trailing[i],4))return w->error;
        }
    }
    if(channel_count==2)for(uint32_t i=0;i<2;i++)if(plus_pack_flag_vector(w,g->stereo_flags+i,(uint32_t)g->subbands)<0)return w->error;
    return w->error?w->error:(int32_t)w->bit_position;
}
