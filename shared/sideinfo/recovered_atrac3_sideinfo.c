#include "recovered_atrac3_sideinfo.h"
typedef struct {uint32_t word,bits;} codeword;
typedef struct {uint32_t count;const codeword* words;} codebook;
#include "tonal_codebooks.inc"
typedef struct {uint8_t* output;uint32_t capacity,bit;} writer;
static uint32_t asr(uint32_t x,unsigned shift){
    if(!shift)return x;
    return (x>>shift)|((x&0x80000000u)?(~0u<<(32-shift)):0);
}
static int put_bits(writer* w,uint32_t value,uint32_t width){
    uint32_t byte=w->bit>>3;if(byte>=w->capacity||w->capacity-byte<2)return -2;
    uint32_t shift=(16u-(w->bit&7u)-width)&31u;
    uint32_t word=value<<shift;
    w->output[byte+1]=(uint8_t)word;w->output[byte]|=(uint8_t)(word>>8);
    w->bit+=width;return 0;
}
static int put_tonal_word(writer* w,codeword value){
    uint32_t byte=w->bit>>3;if(byte>=w->capacity||w->capacity-byte<2)return -2;
    uint32_t word=asr(value.word,(w->bit&7u)+8u);
    w->output[byte+1]=(uint8_t)word;w->output[byte]|=(uint8_t)(word>>8);
    w->bit+=value.bits;return 0;
}
#define PUT(value,width) do{int status=put_bits(w,(value),(width));if(status)return status;}while(0)
static int write_tonal(const sony_atrac3_sideinfo* s,writer* w){
    if(s->tonal_mode>1||s->tonal_group_count>31||!s->tonal_groups)return -1;
    PUT(s->tonal_mode,2);
    for(uint32_t i=0;i<s->tonal_group_count;++i){
        const sony_atrac3_tonal_group* group=s->tonal_groups+i;
        if(group->word_length<2||group->word_length>7||group->coefficients_minus_one>7)return -1;
        uint32_t flags=0;
        for(uint32_t b=0;b<s->subbands;++b){if(group->band_present[b]>1)return -1;flags=2*flags+group->band_present[b];}
        PUT(flags,s->subbands);
        PUT(group->word_length+8*group->coefficients_minus_one,6);
        const codebook* book=tonal_books+2*group->word_length+s->tonal_mode;
        for(uint32_t quarter=0;quarter<4*s->subbands;++quarter){
            if(!group->band_present[quarter/4])continue;
            const sony_atrac3_tonal_window* window=group->window+quarter;
            if(window->count>7||(window->count&&!window->components))return -1;
            PUT(window->count,3);
            for(uint32_t component=0;component<window->count;++component){
                const sony_atrac3_tonal_component* c=window->components+component;
                if(c->scale_factor>63)return -1;
                PUT(c->scale_factor,6);PUT(c->position&63u,6);
                if(!book->words)return -3;
                for(uint32_t k=0;k<=group->coefficients_minus_one;++k){
                    if(c->symbol[k]>=book->count)return -1;
                    int status=put_tonal_word(w,book->words[c->symbol[k]]);if(status)return status;
                }
            }
        }
    }
    return 0;
}
int sony_recovered_atrac3_write_sideinfo(const sony_atrac3_sideinfo* s,
    uint8_t* output,uint32_t capacity,uint32_t byte_offset,uint32_t* end_bit){
    if(!s||!output||!end_bit||s->subbands<1||s->subbands>4||s->quantization_units<1||s->quantization_units>32||s->tonal_group_count>31||byte_offset>0x1fffffffu)return -1;
    uint32_t start=byte_offset;if(start>=capacity||capacity-start<(s->extended_header?2u:1u))return -2;
    uint32_t header=0xa0;
    if(s->extended_header){
        output[start++]=(uint8_t)(s->header_fields[0]*16u+s->header_fields[1]*4u+s->header_fields[2]);
        header=12u+16u*(s->header_fields[3]*4u+s->header_fields[4]);
    }
    output[start]=(uint8_t)(header+s->subbands-1);
    writer state={output,capacity,start*8u+8u};writer* w=&state;
    for(uint32_t band=0;band<s->subbands;++band){
        const sony_atrac3_gain_band* gain=s->gain+band;
        if(gain->count>7)return -1;PUT(gain->count,3);
        for(uint32_t p=0;p<gain->count;++p){
            if(gain->level[p]>15||gain->location[p]>31)return -1;
            PUT(gain->level[p]*32u+gain->location[p],9);
        }
    }
    PUT(s->tonal_group_count,5);
    if(s->tonal_group_count){int status=write_tonal(s,w);if(status)return status;}
    PUT(2u*s->quantization_units-2u,6);
    for(uint32_t qu=0;qu<s->quantization_units;++qu){if(s->word_length[qu]>7)return -1;PUT(s->word_length[qu],3);}
    for(uint32_t qu=0;qu<s->quantization_units;++qu)if(s->word_length[qu]){if(s->scale_factor[qu]>63)return -1;PUT(s->scale_factor[qu],6);}
    *end_bit=w->bit;return 0;
}
