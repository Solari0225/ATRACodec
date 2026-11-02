#include "recovered_atrac3_pcm66.h"
#include "../atrac3_qmf/independent_atrac3_qmf.h"
#include "../peak/recovered_peak_lanes.h"
#include "../allocator/recovered_atrac3_allocator.h"
#include "../frame/recovered_atrac3_frame.h"
#include <string.h>
void sony_recovered_atrac3_pcm66_init(sony_atrac3_pcm66_state *s){
    if(!s)return;memset(s,0,sizeof(*s));s->joint.cutoff=1;
    s->joint.weight_current=15;
    for(uint32_t b=0;b<4;++b){s->joint.previous[b]=-3;s->joint.scale_previous[b]=1;s->joint.scale_current[b]=1;}
}
int sony_recovered_atrac3_pcm66_block(sony_atrac3_pcm66_state *s,
    const int16_t pcm[2048],uint8_t output[195],uint32_t end_bits[2]){
    if(!s||!pcm||!output||!end_bits)return -1;
    for(uint32_t ch=0;ch<2;++ch){
        float input[1024];for(uint32_t i=0;i<1024;++i)input[i]=(float)pcm[2*i+ch];
        sony_atrac3_pcm132_channel *c=s->dsp.channel+ch;
        independent_atrac3_qmf(input,c->qmf_history,c->mdct.samples);
        memcpy(s->joint.samples[ch],c->mdct.samples,4096);
    }
    if(sony_recovered_atrac3_joint_analyze(&s->joint))return -2;
    for(uint32_t b=0;b<4;++b)if(sony_recovered_atrac3_joint_mix_band(&s->joint,b))return -2;
    for(uint32_t ch=0;ch<2;++ch){
        sony_atrac3_pcm132_channel *c=s->dsp.channel+ch;float peaks[4][32];
        memcpy(c->mdct.samples,s->joint.samples[ch],4096);
        if(sony_recovered_peak_lanes_f32(c->mdct.samples,&peaks[0][0]))return -3;
        sony_atrac3_gain_config config;config.extended_header=ch?1133:0;
        for(uint32_t b=0;b<4;++b){config.current[b]=(uint32_t)s->joint.current[b];config.previous[b]=(uint32_t)s->joint.previous[b];}
        memcpy(c->gain.previous_samples,c->mdct.samples+1024,4096);
        if(sony_recovered_atrac3_gain_frame(&c->gain,&config,peaks))return -4;
        memcpy(c->mdct.samples+1024,c->gain.previous_samples,4096);
        for(uint32_t b=0;b<4;++b)c->mdct.gain_index[b]=(uint32_t)c->gain.band[b].level[0];
        if(sony_recovered_atrac3_mdct(&c->mdct,sony_recovered_atrac3_mdct_default_tables()))return -5;
        memcpy(s->joint.samples[ch],c->mdct.samples,4096);
    }
    const sony_atrac3_joint_budget_config budget_config={96,27,12,1133,s->dsp.channel[1].gain.band[0].point_count};
    int32_t budget=sony_recovered_atrac3_joint_left_budget(&s->joint,&budget_config);
    if(budget<0)return -6;
    memset(output,0,195);uint32_t offset=0;
    const uint32_t headers[5]={s->joint.weight_previous,(uint32_t)s->joint.current[0],(uint32_t)s->joint.current[1],(uint32_t)s->joint.current[2],(uint32_t)s->joint.current[3]};
    for(uint32_t ch=0;ch<2;++ch){
        sony_atrac3_pcm132_channel *c=s->dsp.channel+ch;
        sony_atrac3_tonal_analysis_state analysis;memset(&analysis,0,sizeof(analysis));memcpy(analysis.spectrum,c->mdct.samples,4096);
        sony_atrac3_allocation_config config;memset(&config,0,sizeof(config));
        config.maximum_quantization_units=ch?12:27;config.bit_budget=budget;config.extended_header=ch?1133:0;config.header_field=s->joint.weight_previous;config.refinement_trigger=(int32_t)c->gain.previous_low_band_count;
        sony_atrac3_gain_band gains[4];memset(gains,0,sizeof(gains));
        for(uint32_t b=0;b<4;++b){const sony_atrac3_gain_record *r=c->gain.band+b;config.gain_count[b]=gains[b].count=r->point_count;for(uint32_t p=0;p<r->point_count;++p){gains[b].level[p]=(uint32_t)r->level[p];gains[b].location[p]=r->location[p];}}
        if(budget>=40){if(sony_recovered_atrac3_allocate(&config,&analysis)<0)return -7;}
        else{analysis.subbands=1;analysis.quantization_units=1;analysis.tonal_group_count=0;analysis.word_length[0]=0;gains[0].count=0;c->gain.band[0].point_count=0;}
        sony_atrac3_sideinfo info;sony_atrac3_tonal_group groups[2];sony_atrac3_tonal_component components[64];
        if(sony_recovered_atrac3_sideinfo_from_analysis(&analysis,gains,config.extended_header,headers,&info,groups,components)<0)return -8;
        if(sony_recovered_atrac3_write_channel(&info,analysis.spectrum,output,195,offset,end_bits+ch))return -9;
        if(end_bits[ch]>1536)return -10;
        if(!ch){offset=(end_bits[0]+7)/8;budget=(int32_t)((192-offset)*8)-27;}
        else{for(uint32_t first=offset,last=192;first<last;){const uint8_t value=output[first];output[first++]=output[--last];output[last]=value;}}
    }
    return 0;
}
void sony_recovered_atrac3_pcm66_stream_init(sony_atrac3_pcm66_stream *s){if(s){memset(s,0,sizeof(*s));sony_recovered_atrac3_pcm66_init(&s->state);}}
int sony_recovered_atrac3_pcm66_stream_block(sony_atrac3_pcm66_stream *s,
    const int16_t pcm[2048],uint8_t output[195],uint32_t end_bits[2]){
    if(!s||s->drain_step)return -11;const int result=sony_recovered_atrac3_pcm66_block(&s->state,pcm,output,end_bits);if(result)return result;if(!s->primed){s->primed=1;return 0;}return 1;
}
int sony_recovered_atrac3_pcm66_stream_finish(sony_atrac3_pcm66_stream *s,
    uint8_t output[195],uint32_t end_bits[2]){
    if(!s||!s->primed)return -11;if(s->drain_step>=3)return 0;const int16_t pcm[2048]={0};const int result=sony_recovered_atrac3_pcm66_block(&s->state,pcm,output,end_bits);if(result)return result;++s->drain_step;return 1;
}
