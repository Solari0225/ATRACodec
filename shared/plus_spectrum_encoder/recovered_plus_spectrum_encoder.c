#include "recovered_plus_spectrum_encoder.h"
#include "../plus_analysis/independent_plus_phase.h"
#include "../plus_analysis/pcm_tables.inc"
#include <stddef.h>

static void clear(void *data,size_t bytes){
    volatile unsigned char *p=(volatile unsigned char*)data;
    for(size_t i=0;i<bytes;++i)p[i]=0;
}
static void gain(plus_given_gain_band *out,const sony_plus_gain_points *in){
    out->points=in->count;
    for(uint32_t j=0;j<7;++j){out->locations[j]=in->location[j];out->levels[j]=in->level[j];}
}
int sony_recovered_plus_spectrum_default(uint32_t bitrate,sony_plus_spectrum_config *g){
    if(!g)return -1;
    static const uint32_t rates[9]={48000,64000,96000,128000,160000,192000,256000,320000,352000};
    static const uint32_t profiles[9]={13,15,19,23,24,25,27,29,30};
    static const uint32_t units[9]={26,27,27,27,28,29,32,32,32};
    static const uint32_t bands[9]={10,11,11,11,12,13,16,16,16};
    static const uint32_t bytes[9]={280,376,560,744,936,1120,1488,1864,2048};
    uint32_t i=0;while(i<9&&rates[i]!=bitrate)++i;if(i==9)return -1;
    clear(g,sizeof(*g));
    g->preprocess.channels=2;g->preprocess.units=units[i];g->preprocess.subbands=bands[i];
    g->preprocess.backend=1024;g->preprocess.profile_id=(int32_t)profiles[i];
    g->preprocess.sample_rate=44100;g->preprocess.request_mode=2;
    const int32_t unit_mode=i<7?3:2;
    g->preprocess.coherence_start=unit_mode==3?(uint32_t)sony_recovered_plus_coherence_boundary((int32_t)profiles[i],44100):16u;
    g->allocation.initial=(sony_plus_initial_config){2,units[i],bands[i],44100,profiles[i],unit_mode,0};
    g->allocation.scale_groups=(uint32_t)quant_unit_group_u8_33[units[i]]+1;
    g->allocation.simd=1;
    g->frame=(sony_plus_allocated_frame_parameters){1,0,bytes[i],0,0};
    return 0;
}
int sony_recovered_plus_spectrum_initialize(sony_plus_spectrum_state *s){
    if(!s)return -1;clear(s,sizeof(*s));return 0;
}
static int valid(const sony_plus_spectrum_config *g,const sony_plus_spectrum_input *in){
    if(!g||!in||g->preprocess.channels!=2||g->allocation.initial.channels!=2||
       g->preprocess.request_mode!=2||g->allocation.initial.request_mode<0||g->allocation.initial.request_mode>3||
       g->preprocess.units>32||g->preprocess.subbands>16||g->frame.frame_bytes>2048||
       g->frame.frame_bytes<1)return 0;
    if(in->statistics.coherence_boundary<0||in->statistics.coherence_boundary>16||
       in->tones.band_count<0||in->tones.band_count>16)return 0;
    return 1;
}
int32_t sony_recovered_plus_spectrum_allocate(const sony_plus_spectrum_config *g,
    sony_plus_spectrum_state *s,const sony_plus_spectrum_input *in){
    if(!s||!valid(g,in))return -1;
    plus_spectrum_prep_config pc=g->preprocess;
    sony_plus_allocator_config ac=g->allocation;
    /* Original working B4/BC and group count expand with time-feature flags;
     * the immutable factory request remains in g for the next input frame. */
    if(in->evolving_flags&&pc.profile_id>=13)pc.units=32;
    pc.subbands=(uint32_t)quant_unit_band_u8_33[pc.units]+1;
    ac.initial.maximum_units=pc.units;ac.initial.maximum_subbands=pc.subbands;
    ac.initial.special_mode=(int32_t)in->evolving_flags;
    ac.scale_groups=pc.units?(uint32_t)quant_unit_group_u8_33[pc.units]+1:0;
    pc.feature_flags=in->evolving_flags;
    pc.coherence_start=(uint32_t)in->statistics.coherence_boundary;
    for(uint32_t b=0;b<16;++b)pc.source_swap[b]=in->source_swap[b];
    const uint32_t current=s->analysis_frames&1u,previous=1u-current;
    for(uint32_t ch=0;ch<2;++ch){
        plus_spectrum_prep_channel *p=s->preparation+ch;
        p->gain.channel_index=ch;
        for(uint32_t k=0;k<2048;++k){s->normalized[ch].spectrum[k]=in->main_spectrum[ch][k];p->reference_spectrum[k]=in->neutral_spectrum[ch][k];}
        for(uint32_t b=0;b<16;++b){gain(p->gain.bands+b,in->current_gain[ch]+b);gain(p->previous_gain+b,in->previous_gain[ch]+b);}
        for(uint32_t q=0;q<32;++q){
            p->neutral_coarse[q]=s->history[current][ch].neutral_coarse[q];
            p->current_peakiness[q]=s->history[current][ch].peakiness[q];
            p->previous_neutral_coarse[q]=s->history[previous][ch].neutral_coarse[q];
            p->previous_peakiness[q]=s->history[previous][ch].peakiness[q];
        }
    }
    if(plus_spectrum_preprocess(&pc,s->normalized,s->preparation,&s->prepared_shared)||
       plus_feature_prepare_normalized(s->normalized,&s->normalized_shared,pc.units,2,pc.backend))return -1;
    clear(&s->allocated,sizeof(s->allocated));
    sony_plus_allocator_state *a=&s->allocated;
    a->reserve=(int32_t)s->prepared_shared.words[3];
    for(uint32_t ch=0;ch<2;++ch){
        const plus_spectrum_prep_channel *p=s->preparation+ch;
        sony_plus_allocator_channel *c=a->channel+ch;
        c->analysis.maximum_word_length=p->maximum_word_length;
        c->analysis.activity=p->boundary;c->analysis.flag=p->activity;
        c->analysis.scale=p->demand_scale;c->analysis.noise=p->average_tonality;
        c->input.simd=pc.backend!=0;c->sf.channel_index=ch;c->words.channel_index=ch;
        c->gain=p->gain;
        for(uint32_t b=0;b<16;++b){c->previous_gain[b]=in->previous_gain[ch][b];c->window.values[b]=in->window_flags[ch][b];}
        for(uint32_t q=0;q<32;++q){
            c->analysis.significant[q]=p->significant[q];c->analysis.peakiness[q]=p->average_peakiness[q];
            c->input.threshold_factor[q]=s->normalized[ch].factor[q];
            a->stereo.coarse[ch][q]=s->normalized[ch].coarse[q];a->stereo.channel_metric[ch][q]=p->current_peakiness[q];
        }
        for(uint32_t k=0;k<2048;++k){a->stereo.spectrum[ch][k]=s->normalized[ch].spectrum[k];c->input.normalized[k]=s->normalized[ch].spectrum[k];}
    }
    a->stereo.analyzed_bands=in->statistics.coherence_boundary;
    a->stereo.context_policy=(uint32_t)pc.request_mode;
    for(uint32_t b=0;b<16;++b){
        a->stereo.band_flags[b]=in->statistics.coherence[b];a->stereo.band_correlation[b]=in->statistics.contrast[b];
        a->stereo.energy_left[b]=in->statistics.left_energy[b];a->stereo.energy_right[b]=in->statistics.right_energy[b];
        a->stereo.band_score[b]=in->statistics.latest_contrast[b];a->stereo_second[b]=in->source_swap[b];
    }
    for(uint32_t q=0;q<32;++q){a->stereo.base[q]=s->normalized_shared.difference_coarse[q];a->stereo.correlation[q]=s->normalized_shared.difference_peak[q];}
    a->tones.present=in->tones.present;a->tones.amplitude_mode=in->tones.amplitude_mode;
    a->tones.band_count=in->tones.band_count;a->tones.channel_count=2;
    for(uint32_t j=0;j<3;++j)for(uint32_t b=0;b<16;++b)a->tones.flags[j][b+2]=in->tones.flags[j][b];
    for(uint32_t ch=0;ch<2;++ch){
        a->tones.channels[ch].channel_index=ch;
        for(uint32_t b=0;b<16;++b){
            a->tones.channels[ch].bands[b]=in->tones.bands[ch][b];
            for(uint32_t j=0;j<4;++j)a->tones.previous_envelope[ch][b][j]=in->tones.previous_envelope[ch][b][j];
        }
    }
    const int32_t bits=sony_recovered_plus_allocate(&ac,a,(int32_t)(g->frame.frame_bytes*8u-5u));
    if(bits<0)return bits;
    for(uint32_t ch=0;ch<2;++ch)for(uint32_t q=0;q<32;++q){
        s->history[current][ch].neutral_coarse[q]=s->preparation[ch].neutral_coarse[q];
        s->history[current][ch].peakiness[q]=s->preparation[ch].current_peakiness[q];
    }
    ++s->analysis_frames;return bits;
}
int32_t sony_recovered_plus_spectrum_pack(const sony_plus_spectrum_config *g,
    const sony_plus_spectrum_state *s,uint8_t *out,uint32_t capacity,uint32_t *status){
    if(!g||!s||!s->analysis_frames)return -1;
    return sony_recovered_plus_pack_allocated_frame(&g->allocation,&s->allocated,&g->frame,out,capacity,status);
}
int32_t sony_recovered_plus_spectrum_encode(const sony_plus_spectrum_config *g,
    sony_plus_spectrum_state *s,const sony_plus_spectrum_input *in,uint8_t *out,uint32_t capacity,uint32_t *status){
    const int32_t bits=sony_recovered_plus_spectrum_allocate(g,s,in);
    return bits<0?bits:sony_recovered_plus_spectrum_pack(g,s,out,capacity,status);
}
