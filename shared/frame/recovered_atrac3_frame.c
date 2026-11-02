#include "recovered_atrac3_frame.h"
#include "../spectrum/recovered_atrac3_spectrum.h"
int sony_recovered_atrac3_write_channel(const sony_atrac3_sideinfo *info,
    const float spectrum[1024],uint8_t *output,uint32_t capacity,
    uint32_t byte_offset,uint32_t *end_bit)
{
    if(!spectrum)return -1;
    uint32_t bit=0;
    int status=sony_recovered_atrac3_write_sideinfo(info,output,capacity,
                                                  byte_offset,&bit);
    if(status)return status;
    return sony_recovered_atrac3_write_spectrum(spectrum,info->word_length,
        info->scale_factor,info->quantization_units,output,capacity,bit,end_bit);
}
int sony_recovered_atrac3_write_stereo132(const sony_atrac3_sideinfo info[2],
    const float spectra[2][1024],uint8_t *output,uint32_t capacity,
    uint32_t end_bits[2])
{
    return sony_recovered_atrac3_write_independent_stereo(info,spectra,192,
        output,capacity,end_bits);
}
int sony_recovered_atrac3_write_independent_stereo(const sony_atrac3_sideinfo info[2],
    const float spectra[2][1024],uint32_t unit_bytes,uint8_t *output,
    uint32_t capacity,uint32_t end_bits[2])
{
    if(!info||!spectra||!output||!end_bits||info[0].extended_header||
       info[1].extended_header||(unit_bytes!=152&&unit_bytes!=192))return -1;
    if(capacity<unit_bytes*2+3)return -2;
    for(uint32_t i=0;i<unit_bytes*2+3;++i)output[i]=0;
    for(uint32_t channel=0;channel<2;++channel){
        const int status=sony_recovered_atrac3_write_channel(info+channel,
            spectra[channel],output,capacity,channel*unit_bytes,end_bits+channel);
        if(status)return status;
        if(end_bits[channel]>(channel+1)*unit_bytes*8)return -4;
    }
    return 0;
}
int32_t sony_recovered_atrac3_sideinfo_from_analysis(
    const sony_atrac3_tonal_analysis_state *s,
    const sony_atrac3_gain_band gains[4],uint32_t extended,
    const uint32_t header_fields[5],sony_atrac3_sideinfo *info,
    sony_atrac3_tonal_group groups[2],sony_atrac3_tonal_component components[64])
{
    if(!s||!gains||!info||!groups||!components||s->tonal_group_count>2||
       !s->subbands||s->subbands>4||!s->quantization_units||
       s->quantization_units>32||(extended&&!header_fields))return -1;
    info->extended_header=extended;
    for(uint32_t i=0;i<5;++i)info->header_fields[i]=header_fields?header_fields[i]:0;
    info->subbands=s->subbands;
    for(uint32_t b=0;b<4;++b){
        if(gains[b].count>7)return -1;info->gain[b]=gains[b];
    }
    info->tonal_mode=s->tonal_mode;info->tonal_group_count=s->tonal_group_count;
    info->tonal_groups=groups;info->quantization_units=s->quantization_units;
    for(uint32_t q=0;q<32;++q){
        if(q<s->quantization_units&&(s->word_length[q]>7||
           (s->word_length[q]&&s->scale_factor[q]>63)))return -1;
        info->word_length[q]=s->word_length[q];info->scale_factor[q]=s->scale_factor[q];
    }
    uint32_t used=0;
    for(uint32_t g=0;g<s->tonal_group_count;++g){
        const sony_atrac3_tonal_analysis_group *source=s->group+g;
        sony_atrac3_tonal_group *group=groups+g;
        for(uint32_t b=0;b<4;++b)group->band_present[b]=source->band_present[b];
        group->word_length=source->word_length;
        group->coefficients_minus_one=source->coefficients_minus_one;
        for(uint32_t q=0;q<16;++q){
            const sony_atrac3_tonal_slots *window=source->window+q;
            if(window->count>7||used+window->count>64)return -1;
            group->window[q].count=window->count;
            group->window[q].components=components+used;
            for(uint32_t i=0;i<window->count;++i){
                const uint32_t id=window->component_ids[i];if(!id||id>64)return -1;
                components[used++]=s->components[id-1];
            }
        }
    }
    return (int32_t)used;
}
