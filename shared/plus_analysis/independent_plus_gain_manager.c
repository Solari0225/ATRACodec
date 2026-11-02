#include "independent_plus_gain_manager.h"
#include "independent_plus_window.h"
#include "independent_plus_window_stereo.h"
#include <string.h>
static const sony_plus_gain_record empty_record={0};
int sony_recovered_plus_gain_manager(const sony_plus_gain_manager_config *config,
    sony_plus_analysis_channel channel[2],const float contrast[16],
    const float secondary_contrast[16],int32_t coherence[16],int32_t *error)
{
    if(!config||!channel||!contrast||!secondary_contrast||!coherence||!error)return -1;
    if(config->mode!=2||config->policy_force)return -3;
    if(config->channels<1||config->channels>2||config->bands<2||config->bands>16)return -2;
    const int channels=config->channels,bands=config->bands;
    sony_plus_window_result window[2][16];uint32_t flags[2][16];
    double contrast_total=0.;
    for(int b=0;b<bands;++b)contrast_total+=(double)contrast[b];
    const float mean_contrast=(float)(contrast_total/(double)bands);
    sony_plus_window_config wcfg={config->mode,config->policy_force,config->low_band_limit,config->max_qu,config->evolving_flags};
    for(int ch=0;ch<channels;++ch){
        const float *input[16];
        for(int b=0;b<16;++b){
            memcpy(&channel[ch].current_gain[b],&empty_record,sizeof(empty_record));
            window[ch][b].window_flag=0;window[ch][b].low_band_scale=0.;window[ch][b].peak_ratio=0.;
            input[b]=channel[ch].subband[b]+64;
        }
        int status=sony_recovered_plus_select_windows(&wcfg,input,16,
            channel[ch].prior_gain_count,window[ch],flags[ch]);
        if(status)return status;
    }
    if(config->arrangement==3){
        if(channels!=2)return -2;
        int status=sony_recovered_plus_unify_windows(window,flags,contrast,
            (uint32_t)config->coherence_start,config->channel_source,16);
        if(status)return status;
    }
    for(int ch=0;ch<channels;++ch)for(int b=0;b<16;++b){
        sony_plus_gain_record *g=&channel[ch].current_gain[b];
        g->points.window_flag=window[ch][b].window_flag;
        g->low_band_scale=window[ch][b].low_band_scale;g->peak_ratio=window[ch][b].peak_ratio;
        channel[ch].window_flags[b]=flags[ch][b];
    }
    if(!config->force_gain_off){
        for(int b=15;b>=0;--b)for(int ch=0;ch<channels;++ch){
            if(ch==1&&contrast[b]>30.f&&mean_contrast>30.f){
                const double ratio=(double)channel[0].previous_gain[b].maximum/(double)channel[1].previous_gain[b].maximum;
                if(ratio>.5&&ratio<2.)channel[1].previous_gain[b].maximum=channel[0].previous_gain[b].maximum;
            }
            const sony_plus_gain_band_request request={config->mode,channels,config->max_qu,config->coherence_start,b,bands};
            int status=sony_recovered_plus_select_gain_band(&request,channel[ch].subband[b]+191,
                &channel[ch].gain_history[b],channel[ch].previous_gain,channel[ch].current_gain,error);
            if(status)return status;
        }
    }
    if(config->arrangement==3){
        sony_plus_gain_record current[2][16];float previous[2][16];
        for(int ch=0;ch<2;++ch)for(int b=0;b<16;++b){current[ch][b]=channel[ch].current_gain[b];previous[ch][b]=channel[ch].previous_gain[b].maximum;}
        int status=sony_recovered_plus_unify_gain(current,previous,contrast,secondary_contrast,
            coherence,config->max_qu,config->coherence_start,16);
        if(status)return status;
        for(int ch=0;ch<2;++ch)for(int b=0;b<16;++b)channel[ch].current_gain[b]=current[ch][b];
    }
    if(channels==2)for(int b=0;b<16;++b)
        coherence[b]=sony_recovered_plus_gain_points_equal(&channel[0].current_gain[b].points,&channel[1].current_gain[b].points);
    for(int ch=0;ch<channels;++ch){
        for(int b=0;b<bands;++b){
            float state[192];memcpy(state,channel[ch].subband[b],sizeof(state));
            int status=sony_recovered_plus_gain_mdct(state,channel[ch].spectrum+b*128,
                &channel[ch].current_gain[b].points,(uint32_t)(b&1));
            if(status)return status;
            memcpy(channel[ch].subband[b],state,64*sizeof(float));
        }
        for(int b=0;b<2;++b){
            if(!channel[ch].previous_gain[b].points.count&&!channel[ch].current_gain[b].points.count){
                memcpy(channel[ch].neutral_spectrum+b*128,channel[ch].spectrum+b*128,128*sizeof(float));
                memcpy(channel[ch].neutral_overlap[b],channel[ch].subband[b],64*sizeof(float));
            }else{
                float state[192];sony_plus_gain_points points={0};
                memcpy(state,channel[ch].neutral_overlap[b],64*sizeof(float));
                memcpy(state+64,channel[ch].subband[b]+64,128*sizeof(float));
                points.window_flag=channel[ch].current_gain[b].points.window_flag;
                int status=sony_recovered_plus_gain_mdct(state,channel[ch].neutral_spectrum+b*128,&points,(uint32_t)(b&1));
                if(status)return status;
                memcpy(channel[ch].neutral_overlap[b],state,64*sizeof(float));
            }
        }
    }
    return 0;
}
