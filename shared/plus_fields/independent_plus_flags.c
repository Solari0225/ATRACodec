#include "independent_plus_flags.h"
int32_t plus_pack_flag_vector(plus_bit_writer* writer,const plus_given_flag_vector* flags,int32_t count){
    if(!writer||!flags)return PLUS_PACK_INVALID;
    if(flags->present&&flags->explicit_values&&count>0&&!flags->values){writer->error=PLUS_PACK_INVALID;return writer->error;}
    if(plus_pack_bits(writer,flags->present,1))return writer->error;
    if(flags->present){
        if(plus_pack_bits(writer,flags->explicit_values,1))return writer->error;
        if(flags->explicit_values)for(int32_t i=0;i<count;i++)
            if(plus_pack_bits(writer,flags->values[i],1))return writer->error;
    }
    return (int32_t)writer->bit_position;
}
int32_t plus_pack_window_flags(plus_bit_writer* writer,const plus_given_flag_vector* channels,
                               int32_t channel_count,int32_t subband_count){
    if(!writer||(channel_count>0&&!channels))return PLUS_PACK_INVALID;
    for(int32_t channel=0;channel<channel_count;channel++)
        if(plus_pack_flag_vector(writer,channels+channel,subband_count)<0)return writer->error;
    return writer->error?writer->error:(int32_t)writer->bit_position;
}
