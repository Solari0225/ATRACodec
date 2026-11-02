#include "independent_plus_frame.h"
static int emit(void* user,plus_pack_stage stage,uint32_t channels,plus_bit_writer* w){
    const plus_given_frame_fields* f=(const plus_given_frame_fields*)user;
    int32_t value;
    switch(stage){
    case PLUS_PACK_HEADER:value=plus_pack_header(w,f->quantization_units,f->header_flag);break;
    case PLUS_PACK_WORD_LENGTH:value=plus_pack_word_lengths(w,f->word_global,f->word_channels,(int32_t)channels);break;
    case PLUS_PACK_SCALE_FACTOR:value=plus_pack_scale_factors(w,f->active_units,f->scale_channels,(int32_t)channels);break;
    case PLUS_PACK_CODE_TABLE:value=plus_pack_code_tables(w,f->full_code_table,f->active_units,f->code_channels,(int32_t)channels);break;
    case PLUS_PACK_SPECTRUM:value=plus_pack_spectrum(w,f->spectrum_global,f->spectrum_channels,(int32_t)channels);break;
    case PLUS_PACK_WINDOW:value=plus_pack_window_flags(w,f->window_channels,(int32_t)channels,f->window_subbands);break;
    case PLUS_PACK_GAIN:value=plus_pack_gain(w,f->gain_channels,f->gain_locations,(int32_t)channels);break;
    case PLUS_PACK_TONES:value=plus_pack_tones(w,f->tone_global,f->tone_channels,(int32_t)channels);break;
    case PLUS_PACK_NOISE:value=plus_pack_noise(w,f->noise_present,f->noise_level,f->noise_table);break;
    default:w->error=PLUS_PACK_INVALID;return w->error;
    }
    return value<0?(int)value:0;
}
int32_t plus_pack_given_channel_unit(plus_bit_writer* w,const plus_given_frame_fields* f,uint32_t* codec_status){
    if(!w||!f||!codec_status||f->channel_count>2){if(w)w->error=PLUS_PACK_INVALID;return PLUS_PACK_INVALID;}
    return plus_pack_unit(w,f->unit_id,f->channel_count,f->expected_body_bits,emit,(void*)f,codec_status);
}
int32_t plus_pack_given_frame(plus_bit_writer* w,const plus_given_frame_fields* f,uint32_t* codec_status){
    if(!w||!f)return PLUS_PACK_INVALID;
    if(plus_pack_bits(w,0,1))return w->error;
    if(plus_pack_given_channel_unit(w,f,codec_status)<0)return w->error;
    return plus_pack_finish(w,f->target_total_bits);
}
