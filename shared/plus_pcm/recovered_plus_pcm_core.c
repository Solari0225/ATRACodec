#include "recovered_plus_pcm_core.h"
#include <string.h>

int sony_recovered_plus_pcm_core_initialize(uint32_t bitrate,int32_t arrangement,
    sony_plus_pcm_core_state *s){
    if(!s||(arrangement!=2&&arrangement!=3))return -1;
    memset(s,0,sizeof(*s));
    if(sony_recovered_plus_spectrum_default(bitrate,&s->spectrum_config))return -1;
    const sony_plus_initial_config *g=&s->spectrum_config.allocation.initial;
    s->pcm_config=(sony_plus_pcm_config){2,arrangement,(int32_t)g->profile,
        44100,(int32_t)g->maximum_units,0,0};
    if(sony_recovered_plus_pcm_initialize(&s->buffers,s->channel)||
       sony_recovered_plus_spectrum_initialize(&s->spectrum))return -1;
    for(uint32_t k=0;k<3;++k){
        s->feature_order[k]=k;
        if(sony_plus_time_features_initialize(s->feature+k))return -1;
    }
    return 0;
}
static void rotate(sony_plus_pcm_core_state *s){
    /* Original3dc0fc..139 precedes the flags shift and filterbank. Gain
     * envelope histories stay with their channel; two gain records swap. */
    for(uint32_t ch=0;ch<2;++ch)for(uint32_t b=0;b<16;++b){
        const sony_plus_gain_record old=s->channel[ch].current_gain[b];
        s->channel[ch].current_gain[b]=s->channel[ch].previous_gain[b];
        s->channel[ch].previous_gain[b]=old;
    }
    const uint32_t oldest=s->feature_order[0];
    s->feature_order[0]=s->feature_order[1];
    s->feature_order[1]=s->feature_order[2];s->feature_order[2]=oldest;
}
static int spectrum_input(sony_plus_pcm_core_state *s){
    sony_plus_spectrum_input *out=&s->spectrum_input;
    const sony_plus_pcm_statistics *p=&s->buffers.statistics;
    /* The serializer consumes node+14, two rotating records behind the
     * newly analyzed node+1c. Never serialize the new feature immediately. */
    const sony_plus_time_frame *old=s->feature+s->feature_order[0];
    memset(out,0,sizeof(*out));
    out->evolving_flags=s->evolving_flags;
    out->statistics.coherence_boundary=p->coherence_boundary;
    for(uint32_t b=0;b<16;++b){
        out->statistics.coherence[b]=p->coherence[b];
        out->statistics.contrast[b]=p->contrast[1][b];
        out->statistics.left_energy[b]=p->left_energy[0][b];
        out->statistics.right_energy[b]=p->right_energy[0][b];
        out->statistics.latest_contrast[b]=p->latest_contrast[1][b];
        out->source_swap[b]=(uint32_t)s->channel_sources[b];
    }
    out->tones.present=old->common.present;
    out->tones.amplitude_mode=old->common.amplitude_mode;
    out->tones.band_count=(int32_t)old->common.bands;
    static const uint32_t flag_order[3]={0,2,1};
    for(uint32_t j=0;j<3;++j)for(uint32_t b=0;b<16;++b)
        out->tones.flags[j][b]=old->common.flags[flag_order[j]][b+2];
    for(uint32_t ch=0;ch<2;++ch){
        memcpy(out->main_spectrum[ch],s->channel[ch].spectrum,8192);
        memcpy(out->neutral_spectrum[ch],s->channel[ch].neutral_spectrum,8192);
        for(uint32_t b=0;b<16;++b){
            out->current_gain[ch][b]=s->channel[ch].current_gain[b].points;
            out->previous_gain[ch][b]=s->channel[ch].previous_gain[b].points;
            out->window_flags[ch][b]=s->channel[ch].window_flags[b];
            const sony_plus_time_band *in=&old->bands[ch][b];
            plus_given_tone_band *band=&out->tones.bands[ch][b];
            if(in->count>15||in->first_note>48||in->count>48-in->first_note)return -1;
            band->count=in->count;
            for(uint32_t j=0;j<4;++j){
                out->tones.previous_envelope[ch][b][j]=in->envelope[j];
                band->envelope[j]=in->envelope[j+4];
            }
            for(uint32_t j=0;j<in->count;++j){
                const sony_plus_time_note *note=old->common.notes+in->first_note+j;
                band->tones[j]=(plus_given_tone){note->amplitude,note->fine_amplitude,
                    note->phase,note->frequency};
            }
        }
    }
    return 0;
}
int32_t sony_recovered_plus_pcm_core_analyze(sony_plus_pcm_core_state *s,
    const int16_t pcm[4096]){
    if(!s||!pcm||s->failed)return -1;
    for(uint32_t j=0;j<2048;++j)for(uint32_t ch=0;ch<2;++ch)
        s->planar_input[ch][j]=(float)pcm[j*2+ch];
    rotate(s);
    int status=sony_recovered_plus_pcm_begin(&s->pcm_config,s->planar_input,
        &s->buffers,&s->evolving_flags);
    if(status){s->failed=1;return status;}
    const sony_plus_time_frame *previous=s->feature+s->feature_order[1];
    sony_plus_time_frame *current=s->feature+s->feature_order[2];
    float *time_input[32];
    for(uint32_t ch=0;ch<2;++ch)for(uint32_t b=0;b<16;++b)
        time_input[ch*16+b]=s->buffers.time[ch][b]+192;
    /* Original3f51 receives all16 bands even for a10/11-band rate preset. */
    const sony_plus_time_feature_config feature_config={
        s->pcm_config.profile_id,16,2,1024};
    status=sony_plus_time_features_process(&feature_config,previous,current,
        time_input,&s->evolving_flags,&s->error);
    if(status){s->failed=2;return status;}
    /* The real detector updates evolving_flags from its own decisions. It
     * is not derived by OR-ing the resulting common.present field. */
    for(uint32_t ch=0;ch<2;++ch)for(uint32_t b=0;b<16;++b)
        s->channel[ch].prior_gain_count[b]=(int32_t)previous->bands[ch][b].count;
    status=sony_recovered_plus_pcm_finish(&s->pcm_config,&s->buffers,s->channel,
        s->evolving_flags,s->channel_sources,&s->error,&s->frame_info);
    if(status){s->failed=3;return status;}
    if(spectrum_input(s)){s->failed=4;return -1;}
    const int32_t bits=sony_recovered_plus_spectrum_allocate(&s->spectrum_config,
        &s->spectrum,&s->spectrum_input);
    if(bits<0)s->failed=5;
    return bits;
}
int32_t sony_recovered_plus_pcm_core_pack(sony_plus_pcm_core_state *s,
    uint8_t *output,uint32_t capacity){
    if(!s||s->failed)return -1;
    return sony_recovered_plus_spectrum_pack(&s->spectrum_config,&s->spectrum,
        output,capacity,&s->codec_status);
}
