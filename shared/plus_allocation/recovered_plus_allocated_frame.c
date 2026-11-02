#include "recovered_plus_allocated_frame.h"
#include "../plus_frame/independent_plus_frame.h"
int32_t sony_recovered_plus_pack_allocated_frame(const sony_plus_allocator_config *g,const sony_plus_allocator_state *s,
    const sony_plus_allocated_frame_parameters *p,uint8_t *output,uint32_t capacity,uint32_t *status){
    if(!g||!s||!p||!output||!status||p->frame_bytes>capacity||p->frame_bytes>2048||g->initial.channels<1||g->initial.channels>2)return PLUS_PACK_INVALID;
    /* The original lower bit writer ORs new bits into its caller's zeroed
     * frame buffer. This complete-frame boundary owns that initialization. */
    for(uint32_t i=0;i<p->frame_bytes;++i)output[i]=0;
    const uint32_t n=g->initial.channels;plus_given_frame_fields f={0};
    f.unit_id=p->unit_id;f.channel_count=n;f.quantization_units=s->limits.limit_units;f.header_flag=p->header_flag;
    f.expected_body_bits=(uint32_t)s->actual_bits;f.target_total_bits=p->frame_bytes*8u;
    plus_given_word_length_global wg={(int32_t)s->limits.limit_units,0,{0}};plus_given_word_length words[2]={s->channel[0].words,s->channel[1].words};
    f.word_global=&wg;f.word_channels=words;f.active_units=(int32_t)s->limits.active_units;
    plus_given_scale_factor scales[2]={{0}};plus_given_code_table_channel codes[2]={{0}};plus_given_spectrum_channel spectrum[2]={{0}};
    for(uint32_t ch=0;ch<n;++ch){
        const sony_plus_allocator_channel *a=s->channel+ch;const sony_plus_sf_choice_channel *b=&a->sf;
        scales[ch].channel_index=b->channel_index;scales[ch].mode=(uint32_t)b->metadata[0];scales[ch].prefix_count=(uint32_t)b->metadata[1];scales[ch].suffix_width=(uint32_t)b->metadata[2];scales[ch].suffix_base=(uint32_t)b->metadata[3];scales[ch].huffman_id=(uint32_t)b->metadata[4];scales[ch].weight_id=(uint32_t)b->metadata[5];scales[ch].offset_b=(uint32_t)b->metadata[6];scales[ch].offset_a=(uint32_t)b->metadata[7];
        codes[ch].channel_index=b->channel_index;codes[ch].table_flag=a->input.bank;codes[ch].mode=(uint32_t)b->book_metadata[0];codes[ch].partial_count=(uint32_t)b->book_metadata[1];codes[ch].partial_flag=(uint32_t)b->book_metadata[2];spectrum[ch].table_bank=a->input.bank;
        for(uint32_t q=0;q<32;++q){scales[ch].raw[q]=(uint32_t)b->coarse[q];codes[ch].selectors[q]=(uint32_t)b->book_class[q];codes[ch].indices[q]=(uint32_t)a->selected_book[q];spectrum[ch].codebooks[q]=(uint32_t)a->selected_book[q];spectrum[ch].word_lengths[q]=a->analysis.word_length[q];for(uint32_t j=0;j<4;++j)scales[ch].prepared[j][q]=(uint32_t)b->prepared[j][q];}
        for(uint32_t i=0;i<2048;++i)spectrum[ch].coefficients[i]=a->quantized[i];for(uint32_t j=0;j<5;++j)spectrum[ch].trailing[j]=(uint32_t)a->noise_region[j];
    }
    f.scale_channels=scales;f.full_code_table=g->full_table;f.code_channels=codes;
    uint32_t phase[16];for(uint32_t b=0;b<16;++b)phase[b]=(uint32_t)s->stereo.band_output[b];
    plus_given_spectrum_global sg={(int32_t)s->limits.active_units,(int32_t)s->limits.active_subbands,g->full_table,{{s->stereo_modes[1][0],s->stereo_modes[1][1],s->stereo_second},{s->stereo_modes[0][0],s->stereo_modes[0][1],phase}}};
    f.spectrum_global=&sg;f.spectrum_channels=spectrum;f.window_subbands=(int32_t)s->limits.limit_subbands;
    plus_given_flag_vector window[2];for(uint32_t ch=0;ch<2;++ch)window[ch]=(plus_given_flag_vector){s->channel[ch].window.present,s->channel[ch].window.explicit_values,s->channel[ch].window.values};f.window_channels=window;
    plus_given_gain_channel gains[2]={s->channel[0].gain,s->channel[1].gain};plus_given_gain_location_parameters loc[2]={s->channel[0].gain_location,s->channel[1].gain_location};f.gain_channels=gains;f.gain_locations=loc;
    plus_given_tone_global tg={0};tg.present=s->tones.present;tg.amplitude_mode=s->tones.amplitude_mode;tg.band_count=s->tones.band_count;
    for(uint32_t j=0;j<3;++j)tg.stereo_flags[j]=(plus_given_flag_vector){s->tones.flags[j][0],s->tones.flags[j][1],s->tones.flags[j]+2};for(uint32_t j=0;j<256;++j)tg.stereo_map[j]=s->tones.stereo_map[j];f.tone_global=&tg;f.tone_channels=s->tones.channels;
    f.noise_present=g->noise_present;f.noise_level=p->noise_level;f.noise_table=p->noise_table;
    plus_bit_writer writer;plus_pack_init(&writer,output,capacity,0);const int32_t bits=plus_pack_given_frame(&writer,&f,status);return bits<0?bits:bits/8;
}
