#include "recovered_atrac3_pcm132.h"
#include "../atrac3_qmf/independent_atrac3_qmf.h"
#include "../peak/recovered_peak_lanes.h"
#include "../allocator/recovered_atrac3_allocator.h"
#include "../frame/recovered_atrac3_frame.h"
#include <string.h>
void sony_recovered_atrac3_pcm132_init(sony_atrac3_pcm132_state *state){
    if(state)memset(state,0,sizeof(*state));
}
int sony_recovered_atrac3_pcm132_block(sony_atrac3_pcm132_state *state,
    const int16_t pcm[2048],uint8_t output[387],uint32_t end_bits[2])
{
    return sony_recovered_atrac3_stereo_block(state,pcm,132000,output,387,end_bits);
}
int sony_recovered_atrac3_stereo_block(sony_atrac3_independent_stereo_state *state,
    const int16_t pcm[2048],uint32_t nominal_bitrate,uint8_t *output,
    uint32_t capacity,uint32_t end_bits[2])
{
    if(!state||!pcm||!output||!end_bits)return -1;
    uint32_t unit_bytes,maximum_qu;
    if(nominal_bitrate==105000){unit_bytes=152;maximum_qu=28;}
    else if(nominal_bitrate==132000){unit_bytes=192;maximum_qu=30;}
    else return -8;
    if(capacity<unit_bytes*2+3)return -2;
    sony_atrac3_sideinfo info[2];
    sony_atrac3_tonal_group groups[2][2];
    sony_atrac3_tonal_component components[2][64];
    float spectra[2][1024];
    const sony_atrac3_gain_config gain_config={0,{0,0,0,0},{0,0,0,0}};
    const uint32_t header_fields[5]={0,0,0,0,0};
    for(uint32_t ch=0;ch<2;++ch){
        sony_atrac3_pcm132_channel *c=state->channel+ch;
        float input[1024],peaks[4][32];
        for(uint32_t i=0;i<1024;++i)input[i]=(float)pcm[2*i+ch];
        independent_atrac3_qmf(input,c->qmf_history,c->mdct.samples);
        if(sony_recovered_peak_lanes_f32(c->mdct.samples,&peaks[0][0]))return -2;
        memcpy(c->gain.previous_samples,c->mdct.samples+1024,4096);
        if(sony_recovered_atrac3_gain_frame(&c->gain,&gain_config,peaks))return -3;
        memcpy(c->mdct.samples+1024,c->gain.previous_samples,4096);
        for(uint32_t band=0;band<4;++band)
            c->mdct.gain_index[band]=(uint32_t)c->gain.band[band].level[0];
        if(sony_recovered_atrac3_mdct(&c->mdct,sony_recovered_atrac3_mdct_default_tables()))return -4;
        sony_atrac3_tonal_analysis_state analysis;
        memset(&analysis,0,sizeof(analysis));memcpy(analysis.spectrum,c->mdct.samples,4096);
        sony_atrac3_allocation_config allocation={0,0,0,0,0,{0,0,0,0}};
        allocation.maximum_quantization_units=maximum_qu;
        allocation.bit_budget=(int32_t)(unit_bytes*8)-19;
        allocation.refinement_trigger=(int32_t)c->gain.previous_low_band_count;
        sony_atrac3_gain_band gains[4];
        memset(gains,0,sizeof(gains));
        for(uint32_t band=0;band<4;++band){
            const sony_atrac3_gain_record *r=c->gain.band+band;
            allocation.gain_count[band]=gains[band].count=r->point_count;
            for(uint32_t point=0;point<r->point_count;++point){
                gains[band].level[point]=(uint32_t)r->level[point];
                gains[band].location[point]=r->location[point];
            }
        }
        if(sony_recovered_atrac3_allocate(&allocation,&analysis)<0)return -5;
        if(sony_recovered_atrac3_sideinfo_from_analysis(&analysis,gains,0,
            header_fields,info+ch,groups[ch],components[ch])<0)return -6;
        memcpy(spectra[ch],analysis.spectrum,4096);
    }
    return sony_recovered_atrac3_write_independent_stereo(info,spectra,unit_bytes,
        output,capacity,end_bits);
}
void sony_recovered_atrac3_pcm132_stream_init(sony_atrac3_pcm132_stream *stream){
    (void)sony_recovered_atrac3_stereo_stream_init(stream,132000);
}
int sony_recovered_atrac3_pcm132_stream_block(sony_atrac3_pcm132_stream *stream,
    const int16_t pcm[2048],uint8_t output[387],uint32_t end_bits[2])
{
    if(!stream||stream->nominal_bitrate!=132000)return -8;
    return sony_recovered_atrac3_stereo_stream_block(stream,pcm,output,387,end_bits);
}
int sony_recovered_atrac3_stereo_stream_init(sony_atrac3_independent_stereo_stream *stream,
    uint32_t nominal_bitrate)
{
    if(!stream||(nominal_bitrate!=105000&&nominal_bitrate!=132000))return -8;
    memset(stream,0,sizeof(*stream));stream->nominal_bitrate=nominal_bitrate;return 0;
}
int sony_recovered_atrac3_stereo_stream_block(sony_atrac3_independent_stereo_stream *stream,
    const int16_t pcm[2048],uint8_t *output,uint32_t capacity,uint32_t end_bits[2])
{
    if(!stream||stream->drain_step)return -7;
    const int status=sony_recovered_atrac3_stereo_block(&stream->dsp,pcm,
        stream->nominal_bitrate,output,capacity,end_bits);
    if(status)return status;
    if(!stream->primed){stream->primed=1;return 0;}
    return 1;
}
int sony_recovered_atrac3_pcm132_stream_finish(sony_atrac3_pcm132_stream *stream,
    uint8_t output[387],uint32_t end_bits[2])
{
    if(!stream||stream->nominal_bitrate!=132000)return -8;
    return sony_recovered_atrac3_stereo_stream_finish(stream,output,387,end_bits);
}
int sony_recovered_atrac3_stereo_stream_finish(sony_atrac3_independent_stereo_stream *stream,
    uint8_t *output,uint32_t capacity,uint32_t end_bits[2])
{
    if(!stream||!stream->primed)return -7;
    if(stream->drain_step>=3)return 0;
    const int16_t silence[2048]={0};
    const int status=sony_recovered_atrac3_stereo_block(&stream->dsp,silence,
        stream->nominal_bitrate,output,capacity,end_bits);
    if(status)return status;
    ++stream->drain_step;return 1;
}
