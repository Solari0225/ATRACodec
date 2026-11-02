#include "independent_plus_packer.h"
#include <limits.h>

static int reserve_bits(plus_bit_writer* w,uint32_t count){
    if(!w)return PLUS_PACK_INVALID;
    if(w->error)return w->error;
    const uint64_t end=(uint64_t)w->bit_position+count;
    if(end>INT32_MAX||end>(uint64_t)w->capacity*8u){w->error=PLUS_PACK_NO_SPACE;return w->error;}
    return 0;
}
int plus_pack_init(plus_bit_writer* w,uint8_t* data,size_t capacity,uint32_t bit_position){
    if(!w)return PLUS_PACK_INVALID;
    w->data=data;w->capacity=capacity;w->bit_position=bit_position;w->error=0;
    if(!data||capacity>(size_t)INT32_MAX/8||bit_position>INT32_MAX||(uint64_t)bit_position>(uint64_t)capacity*8u){w->error=PLUS_PACK_INVALID;return w->error;}
    return 0;
}
int plus_pack_bits(plus_bit_writer* w,uint32_t value,unsigned width){
    if(!w)return PLUS_PACK_INVALID;
    if(width>32){w->error=PLUS_PACK_INVALID;return w->error;}
    if(reserve_bits(w,width))return w->error;
    unsigned remaining=width;
    while(remaining){
        const unsigned byte_bit=w->bit_position&7u;
        const unsigned available=8-byte_bit;
        const unsigned take=remaining<available?remaining:available;
        remaining-=take;
        const uint32_t low_mask=(1u<<take)-1u;
        const uint8_t part=(uint8_t)(((value>>remaining)&low_mask)<<(available-take));
        w->data[w->bit_position>>3]|=part;
        w->bit_position+=take;
    }
    return 0;
}
int32_t plus_pack_unit_id(plus_bit_writer* w,uint32_t raw_unit_id){
    if(plus_pack_bits(w,raw_unit_id,2))return w?w->error:PLUS_PACK_INVALID;
    return 2;
}
int32_t plus_pack_header(plus_bit_writer* w,uint32_t raw_quant_unit_count,uint32_t raw_header_flag){
    if(reserve_bits(w,6))return w?w->error:PLUS_PACK_INVALID;
    plus_pack_bits(w,raw_quant_unit_count-1u,5);
    plus_pack_bits(w,raw_header_flag,1);
    return (int32_t)w->bit_position;
}
int32_t plus_pack_noise(plus_bit_writer* w,uint32_t raw_flag,uint32_t raw_level,uint32_t raw_table){
    /* Native conditional compares the complete stored word with1, not just
     * the low bit written to the stream. Preserve it even for synthetic edges.
     */
    if(reserve_bits(w,raw_flag==1?9:1))return w?w->error:PLUS_PACK_INVALID;
    plus_pack_bits(w,raw_flag,1);
    if(raw_flag==1){plus_pack_bits(w,raw_level,4);plus_pack_bits(w,raw_table,4);}
    return (int32_t)w->bit_position;
}
int32_t plus_pack_finish(plus_bit_writer* w,uint32_t target_total_bits){
    if(!w)return PLUS_PACK_INVALID;
    const uint64_t aligned=((uint64_t)w->bit_position+2u+7u)&~(uint64_t)7u;
    const uint64_t target=(uint64_t)target_total_bits&~(uint64_t)7u;
    const uint64_t final=aligned>target?aligned:target;
    if(final>INT32_MAX){w->error=PLUS_PACK_NO_SPACE;return w->error;}
    if(reserve_bits(w,(uint32_t)final-w->bit_position))return w->error;
    plus_pack_bits(w,3,2);
    const unsigned alignment_bits=(8u-(w->bit_position&7u))&7u;
    plus_pack_bits(w,0,alignment_bits);
    while(w->bit_position<final)plus_pack_bits(w,1,8);
    return (int32_t)w->bit_position;
}
int32_t plus_pack_body(plus_bit_writer* w,uint32_t channel_count,
                      uint32_t expected_body_bits,plus_pack_emit_stage emit,
                      void* user,uint32_t* codec_status){
    if(!w||!emit||!codec_status)return PLUS_PACK_INVALID;
    if(w->error)return w->error;
    const uint32_t start=w->bit_position;
    for(unsigned stage=0;stage<PLUS_PACK_STAGE_COUNT;stage++){
        if(emit(user,(plus_pack_stage)stage,channel_count,w)){
            if(!w->error)w->error=PLUS_PACK_STAGE_FAILED;
            return w->error;
        }
        if(w->error)return w->error;
    }
    if(w->bit_position-start!=expected_body_bits)*codec_status=0x11b;
    return (int32_t)w->bit_position;
}
int32_t plus_pack_unit(plus_bit_writer* w,uint32_t raw_unit_id,
                      uint32_t channel_count,uint32_t expected_body_bits,
                      plus_pack_emit_stage emit,void* user,uint32_t* codec_status){
    if(plus_pack_unit_id(w,raw_unit_id)<0)return w?w->error:PLUS_PACK_INVALID;
    return plus_pack_body(w,channel_count,expected_body_bits,emit,user,codec_status);
}
