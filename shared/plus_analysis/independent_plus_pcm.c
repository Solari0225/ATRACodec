#include "independent_plus_pcm.h"
#include "independent_plus_filter.h"
#include "independent_plus_phase.h"
#include <stddef.h>
#include <string.h>
#include "pcm_tables.inc"
_Static_assert(sizeof(sony_plus_pcm_statistics)==0x684,"Typed stereo statistics layout");
_Static_assert(offsetof(sony_plus_pcm_statistics,contrast)==0x44,"Contrast triplet");
_Static_assert(offsetof(sony_plus_pcm_statistics,latest_contrast)==0x284,"Raw contrast history");
_Static_assert(offsetof(sony_plus_pcm_statistics,phase_coefficient)==0x444,"Phase coefficients");
_Static_assert(offsetof(sony_plus_pcm_statistics,weights)==0x4c4,"Stereo weights");
static int valid(const sony_plus_pcm_config *config){return config&&config->channels>=1&&config->channels<=2&&config->profile_id>=0&&config->profile_id<32&&config->requested_quant_units>=0&&config->requested_quant_units<=32&&config->policy_force==0&&(config->arrangement!=3||config->channels==2);}
static float initial_contrast(void){const uint32_t bits=0x426fffff;float result;memcpy(&result,&bits,4);return result;}
int sony_recovered_plus_pcm_initialize(sony_plus_pcm_buffers *buffers,sony_plus_analysis_channel channel[2]){
    if(!buffers||!channel)return -1;
    unsigned char *p=(unsigned char*)buffers;for(size_t i=0;i<sizeof(*buffers);++i)p[i]=0;
    p=(unsigned char*)channel;for(size_t i=0;i<2*sizeof(*channel);++i)p[i]=0;
    for(uint32_t b=0;b<16;++b){
        for(uint32_t frame=0;frame<3;++frame)buffers->statistics.contrast[frame][b]=initial_contrast();
        for(uint32_t frame=0;frame<7;++frame)buffers->statistics.latest_contrast[frame][b]=initial_contrast();
        for(uint32_t ch=0;ch<2;++ch){
            buffers->statistics.weights.old_weight[ch][b]=buffers->statistics.weights.new_weight[ch][b]=.5f;
            for(uint32_t j=0;j<32;++j)channel[ch].gain_history[b].envelope[j]=channel[ch].gain_history[b].difference[j]=4.f;
            sony_plus_gain_record r={0};r.maximum=r.weighted_maximum=r.selected_baseline=r.difference_maximum=r.difference_baseline=4.f;r.envelope_sum=r.difference_envelope_sum=128.f;
            channel[ch].previous_gain[b]=channel[ch].current_gain[b]=r;
        }
    }return 0;
}
int sony_recovered_plus_pcm_begin(const sony_plus_pcm_config *config,const float pcm[2][2048],sony_plus_pcm_buffers *buffers,uint32_t *evolving_flags){
    if(!valid(config)||!pcm||!buffers||!evolving_flags)return -1;
    sony_plus_pcm_statistics *s=&buffers->statistics;
    for(uint32_t b=0;b<16;++b){
        s->contrast[0][b]=s->contrast[1][b];s->contrast[1][b]=s->contrast[2][b];s->contrast[2][b]=initial_contrast();
        s->left_energy[0][b]=s->left_energy[1][b];s->left_energy[1][b]=0.;
        s->right_energy[0][b]=s->right_energy[1][b];s->right_energy[1][b]=0.;
        s->source_flags[0][b]=s->source_flags[1][b];s->source_flags[1][b]=0;
        for(uint32_t frame=0;frame<3;++frame)s->latest_contrast[frame][b]=s->latest_contrast[frame+1][b];
        s->latest_contrast[3][b]=initial_contrast();
    }
    *evolving_flags=(*evolving_flags&15u)*2u;
    for(int32_t ch=0;ch<config->channels;++ch){
        for(uint32_t b=0;b<16;++b){
            for(uint32_t i=0;i<256;++i)buffers->time[ch][b][64+i]=buffers->time[ch][b][192+i];
            for(uint32_t i=320;i<448;++i)buffers->time[ch][b][i]=0.;
        }
        float filtered[16][128];
        int status=sony_recovered_plus_filter(pcm[ch],buffers->filter_history[ch],filtered);if(status)return status;
        for(uint32_t b=0;b<16;++b)memcpy(buffers->time[ch][b]+320,filtered[b],128*sizeof(float));
    }
    if(config->channels==2){
        float frame[2][16][128];for(uint32_t ch=0;ch<2;++ch)for(uint32_t b=0;b<16;++b)memcpy(frame[ch][b],buffers->time[ch][b]+320,128*sizeof(float));
        int status=sony_recovered_plus_stereo_contrast(frame[0],frame[1],s->latest_contrast[3],16);if(status)return status;
        if(config->arrangement==3){
            status=sony_recovered_plus_phase_mix(frame[0],frame[1],s->phase_coefficient[0],&s->coherence_boundary,config->profile_id,config->sample_rate,16);if(status)return status;
            status=sony_recovered_plus_stereo_weights(frame[0],frame[1],s->latest_contrast[3],&s->weights,s->coherence_boundary,16);if(status)return status;
        }else s->coherence_boundary=16;
        for(uint32_t ch=0;ch<2;++ch)for(uint32_t b=0;b<16;++b)memcpy(buffers->time[ch][b]+320,frame[ch][b],128*sizeof(float));
    }else s->coherence_boundary=16;
    return 0;
}
int sony_recovered_plus_pcm_finish(const sony_plus_pcm_config *config,sony_plus_pcm_buffers *buffers,sony_plus_analysis_channel channel[2],uint32_t evolving_flags,int32_t channel_sources[16],int32_t *error,sony_plus_pcm_frame_info *info){
    if(!valid(config)||!buffers||!channel||!channel_sources||!error||!info)return -1;
    int32_t qu=config->requested_quant_units;
    if(evolving_flags&&((config->channels==2&&config->profile_id>=13)||(config->channels==1&&config->profile_id>=11)))qu=32;
    const int32_t coded_bands=(int32_t)quant_unit_band_u8_33[qu]+1;
    const int32_t groups=qu>0?(int32_t)quant_unit_group_u8_33[qu]+1:0;
    int32_t metric_bands=coded_bands;
    if(config->channels==2&&coded_bands<16&&(evolving_flags&1u))metric_bands=16;
    sony_plus_pcm_statistics *s=&buffers->statistics;
    if(config->channels==2){
        float frame[2][16][128];for(uint32_t ch=0;ch<2;++ch)for(uint32_t b=0;b<16;++b)memcpy(frame[ch][b],buffers->time[ch][b]+192,128*sizeof(float));
        sony_plus_stereo_metrics m;
        memcpy(m.contrast,s->contrast[2],64);memcpy(m.left_energy,s->left_energy[1],64);memcpy(m.right_energy,s->right_energy[1],64);
        memcpy(m.recent_left,s->recent_left,64);memcpy(m.recent_right,s->recent_right,64);memcpy(m.recent_difference,s->recent_difference,64);
        int status=sony_recovered_plus_stereo_metrics(frame[0],frame[1],&m,(uint32_t)metric_bands);if(status)return status;
        memcpy(s->contrast[2],m.contrast,64);memcpy(s->left_energy[1],m.left_energy,64);memcpy(s->right_energy[1],m.right_energy,64);
        memcpy(s->recent_left,m.recent_left,64);memcpy(s->recent_right,m.recent_right,64);memcpy(s->recent_difference,m.recent_difference,64);
        status=sony_recovered_plus_select_source_flags(s->contrast[2],s->left_energy[1],s->right_energy[1],s->source_flags[0],s->source_flags[1],(uint32_t)metric_bands);if(status)return status;
        /*3e54b0selects the next frame's flag at+244.3e5530copies the
         * already-delayed+204flag into this frame's settings+50. */
        memcpy(channel_sources,s->source_flags[0],(size_t)metric_bands*sizeof(int32_t));
    }
    sony_plus_gain_manager_config manager={2,config->channels,config->arrangement,config->profile_id,coded_bands,coded_bands,evolving_flags,config->force_gain_off,config->policy_force,s->coherence_boundary,{0}};
    memcpy(manager.channel_source,channel_sources,64);
    for(int32_t ch=0;ch<config->channels;++ch)for(uint32_t b=0;b<16;++b){memcpy(channel[ch].subband[b],buffers->time[ch][b],324*sizeof(float));memcpy(channel[ch].neutral_overlap[b],buffers->neutral_overlap[ch][b],64*sizeof(float));}
    int status=sony_recovered_plus_gain_manager(&manager,channel,s->contrast[1],s->contrast[0],s->coherence,error);if(status)return status;
    for(int32_t ch=0;ch<config->channels;++ch)for(uint32_t b=0;b<16;++b){memcpy(buffers->time[ch][b],channel[ch].subband[b],64*sizeof(float));memcpy(buffers->neutral_overlap[ch][b],channel[ch].neutral_overlap[b],64*sizeof(float));}
    info->quant_units=qu;info->coded_bands=coded_bands;info->quant_groups=groups;info->stereo_metric_bands=metric_bands;
    return 0;
}
