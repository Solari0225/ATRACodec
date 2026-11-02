/* Composition recovered from ordinary3dffc0/3da9a0/3f0dc0/3f0f60. */
#include "plus_core.h"
#include "plus_dequant_float.h"
#include "plus_gain_float.h"
#include "plus_imdct_float.h"
#include "plus_noise_float.h"
#include "../common/plus_power_float.h"
#include <string.h>

static const uint16_t starts[33]={0,16,32,48,64,80,96,112,128,160,192,224,256,
    288,320,352,384,448,512,576,640,704,768,896,1024,1152,1280,1408,1536,1664,1792,1920,2048};
static const uint8_t power_groups[16]={0,1,1,2,2,2,3,3,3,3,3,4,4,4,4,4};

void atracodec_plus_core_reset(atracodec_plus_core_state *state)
{
    if(state)memset(state,0,sizeof(*state));
}
static atracodec_plus_tone_band tone_view(const atracodec_plus_tone_fields *t,unsigned ch,unsigned band)
{
    atracodec_plus_tone_band out;
    const atracodec_plus_tone_fields_band *in=&t->band[ch][band];
    memcpy(out.envelope,in->envelope,sizeof(out.envelope));
    out.count=in->count;out.notes=t->notes+in->first_note;return out;
}
int atracodec_plus_core_render(const atracodec_plus_frame *frame,
    atracodec_plus_core_state *state,float output[2][2048])
{
    int32_t word[2][32];int16_t quantized[2][2048];
    float spectrum[2][2048],time[2][16][128];
    atracodec_plus_tone_fields current_tones;
    unsigned ch,q,band,i;uint32_t seed_sum=0;
    if(!frame||!state||!output||frame->channels<1||frame->channels>2||
       frame->units>32||frame->frame_subbands<1||frame->frame_subbands>16||
       frame->coefficient_subbands>frame->frame_subbands||frame->tones.note_count>48)return -1;
    memcpy(word,frame->word_length,sizeof(word));memcpy(quantized,frame->quantized,sizeof(quantized));
    for(ch=0;ch<frame->channels;++ch)for(q=0;q<frame->units;++q)seed_sum+=(uint32_t)frame->scale_factor[ch][q];
    seed_sum&=0x3fc;
    if(frame->channels==2)for(q=0;q<frame->units;++q)
        if(word[1][q]==0&&word[0][q]>0&&frame->codebook[1][q]==0){
            memcpy(quantized[1]+starts[q],quantized[0]+starts[q],(starts[q+1]-starts[q])*sizeof(int16_t));
            word[1][q]=word[0][q];
        }
    memset(spectrum,0,sizeof(spectrum));
    for(ch=0;ch<frame->channels;++ch){
        for(q=0;q<frame->units;++q)if(word[ch][q]>0)
            if(atracodec_plus_dequant_float(quantized[ch]+starts[q],spectrum[ch]+starts[q],
                    (uint32_t)word[ch][q],(uint32_t)frame->scale_factor[ch][q],starts[q+1]-starts[q]))return -1;
        for(band=0;band<frame->coefficient_subbands;++band){
            const unsigned reference=ch^((frame->channels==2&&frame->swap[band])?1u:0u);
            atracodec_plus_power_config config;
            memcpy(config.target_word_length,word[ch],sizeof(config.target_word_length));
            memcpy(config.target_scale_factor,frame->scale_factor[ch],sizeof(config.target_scale_factor));
            config.reference_current_gain=frame->gain[reference][band];
            config.reference_previous_gain=state->previous_gain[reference][band];
            config.band=band;config.power_level=frame->power_level[reference][power_groups[band]];
            config.seed16=(uint16_t)((seed_sum+band*128)&0x3fc);config.reserved=0;
            if(atracodec_plus_apply_power_float(&config,spectrum[ch]))return -1;
        }
    }
    if(frame->channels==2)for(band=0;band<frame->coefficient_subbands;++band){
        if(frame->swap[band])for(i=0;i<128;++i){float v=spectrum[0][band*128+i];spectrum[0][band*128+i]=spectrum[1][band*128+i];spectrum[1][band*128+i]=v;}
        if(frame->negative[band])for(i=0;i<128;++i)spectrum[1][band*128+i]=-spectrum[1][band*128+i];
    }
    if(frame->mute==1)memset(spectrum,0,sizeof(spectrum));
    current_tones=frame->tones;
    /* The native tonal reader changes only the prior right coded envelope
     * when sharing is enabled, before its current-band stereo swap. */
    if(frame->channels==2&&current_tones.present)for(band=0;band<current_tones.bands;++band)
        if(current_tones.shared[band])memcpy(state->previous_tones.band[1][band].envelope+4,
            state->previous_tones.band[0][band].envelope+4,4*sizeof(int32_t));
    memset(time,0,sizeof(time));
    for(ch=0;ch<frame->channels;++ch){
        const float *synthesis_bands[16];float *noise_bands[16];
        for(band=0;band<frame->frame_subbands;++band){
            const atracodec_plus_gain *previous=&state->previous_gain[ch][band];
            const float scalar=atracodec_plus_gain_scale_float(&frame->gain[ch][band]);
            if(previous->window_flag>1)return -1;
            atracodec_plus_imdct_float(spectrum[ch]+band*128,time[ch][band],state->overlap[ch][band],
                (int)previous->window_flag,scalar,(int)(band&1));
            if(previous->count&&atracodec_plus_apply_gain_float(previous,time[ch][band]))return -1;
        }
        for(band=frame->frame_subbands;band<16;++band)memset(state->overlap[ch][band],0,64*sizeof(float));
        if(current_tones.present||state->previous_tones.present)for(band=0;band<16;++band){
            atracodec_plus_tone_fields_band *b=&current_tones.band[ch][band];
            const atracodec_plus_tone_fields_band *a=&state->previous_tones.band[ch][band];
            if(b->count>15||b->first_note>48||b->count>48-b->first_note||
               a->count>15||a->first_note>48||a->count>48-a->first_note)return -1;
            if(atracodec_plus_tone_envelope(b->envelope,a->envelope))return -1;
            if(a->count||b->count){
                atracodec_plus_tone_band av=tone_view(&state->previous_tones,ch,band),bv=tone_view(&current_tones,ch,band);
                float tonal[128];
                if(atracodec_plus_tone_pair(&av,(int)state->previous_tones.amplitude_mode,(int)state->previous_tones.invert[band],
                    &bv,(int)current_tones.amplitude_mode,(int)current_tones.invert[band],tonal,(int)ch,1))return -1;
                for(i=0;i<128;++i){volatile float sum=time[ch][band][i]+tonal[i];time[ch][band][i]=sum;}
            }
        }
        for(band=0;band<16;++band){synthesis_bands[band]=time[ch][band];noise_bands[band]=time[ch][band];}
        if(frame->noise_present&&atracodec_plus_add_noise_float(noise_bands,frame->noise_level,frame->noise_index+ch*16))return -1;
        if(atracodec_plus_ipqf_float(synthesis_bands,output[ch],state->synthesis+ch))return -1;
    }
    for(ch=0;ch<frame->channels;++ch)for(band=0;band<16;++band){
        state->previous_gain[ch][band]=frame->gain[ch][band];
        state->previous_gain[ch][band].window_flag=frame->window[ch][band];
    }
    state->previous_tones=current_tones;
    return 0;
}
