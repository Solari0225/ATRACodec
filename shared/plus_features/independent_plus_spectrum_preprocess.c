#include "independent_plus_spectrum_preprocess.h"
typedef union prep_bits {float f;uint32_t u;} prep_bits;
static float value(uint32_t bits){prep_bits p;p.u=bits;return p.f;}
static int finite_value(float f){prep_bits p;p.f=f;return (p.u&0x7f800000u)!=0x7f800000u;}
static const uint8_t coherence_unit_boundary[17]={0,8,12,16,18,20,22,23,24,25,26,27,28,29,30,31,32};
static void scale(float* a,uint32_t n,float multiplier){
    for(uint32_t i=0;i<n;++i)a[i]=(float)((double)a[i]*(double)multiplier);
}
static void exchange(float* a,float* b){
    for(uint32_t i=0;i<128;++i){const float temp=a[i];a[i]=b[i];b[i]=temp;}
}
static int16_t initial_limit(uint32_t channels,int32_t profile){
    if(channels==2)return profile>=19?7:profile>=15?6:profile>=11?5:profile>=5?4:profile>=4?3:2;
    return profile>=15?7:profile>=13?6:profile>=7?5:profile>=5?4:profile>=4?3:2;
}
static int valid_gain(const plus_given_gain_band b[16]){
    for(uint32_t k=0;k<16;++k){
        if(b[k].points>7)return 0;
        for(uint32_t p=0;p<b[k].points;++p)if(b[k].levels[p]>15)return 0;
    }
    return 1;
}
int plus_spectrum_preprocess(const plus_spectrum_prep_config* cfg,
    plus_feature_channel core[2],plus_spectrum_prep_channel c[2],
    plus_spectrum_prep_shared* shared){
    if(!cfg||!core||!c||!shared||cfg->channels<1||cfg->channels>2||cfg->units>32||
        cfg->subbands>16||cfg->profile_id<0||cfg->profile_id>31||cfg->coherence_start>16)return -1;
    const uint32_t channels=cfg->channels,units=cfg->units;
    const int32_t mode=cfg->request_mode,profile=cfg->profile_id;
    for(uint32_t ch=0;ch<channels;++ch){
        if(!valid_gain(c[ch].gain.bands)||!valid_gain(c[ch].previous_gain))return -1;
        for(uint32_t k=0;k<2048;++k)
            if(!finite_value(core[ch].spectrum[k])||!finite_value(c[ch].reference_spectrum[k]))return -1;
        for(uint32_t q=0;q<units;++q)
            if(c[ch].previous_neutral_coarse[q]<0||c[ch].previous_neutral_coarse[q]>63||
               !finite_value(c[ch].previous_peakiness[q]))return -1;
    }
    shared->header[0]=mode==2?0u:1u;
    shared->header[1]=0;shared->header[2]=0;shared->header[3]=0;
    shared->header[4]=channels==1?0u:1u;shared->header[5]=0;
    shared->words[0]=1;shared->words[1]=0;shared->words[2]=1;shared->words[3]=mode==2?16u:4u;
    for(uint32_t q=0;q<units;++q){shared->stereo[q]=0;shared->trailing[q]=0;}
    for(uint32_t ch=0;ch<channels;++ch){
        plus_spectrum_gain_header(&c[ch].gain,c[ch].previous_gain,cfg->subbands,&c[ch].activity);
        for(uint32_t q=0;q<32;++q){c[ch].word_length[q]=0;c[ch].book[q]=0;core[ch].coarse[q]=0;}
        c[ch].maximum_word_length=initial_limit(channels,profile);
    }
    if(channels==1&&cfg->force_gain_off)c[0].maximum_word_length=7;
    if((channels==2&&profile<=11)||(channels==1&&profile==9)){
        const float multiplier=value(0x3f642a00u);
        for(uint32_t ch=0;ch<channels;++ch){
            scale(core[ch].spectrum,2048,multiplier);
            scale(c[ch].reference_spectrum,mode==2?256:2048,multiplier);
        }
    }
    if((channels==2&&profile>=13)||(channels==1&&profile>=11)){
        const uint32_t f=cfg->feature_flags;
        int selected;
        if(mode==2)selected=(f&0x1eu)!=0&&(f&0x1eu)!=0x1eu;
        else selected=(f&0xcu)==4||(f&0x18u)==8||(f&0x30u)==0x10||
            (f&0x18u)==0x10||(f&0x30u)==0x20||(f&0x60u)==0x40;
        if(selected)for(uint32_t ch=0;ch<channels;++ch){
            scale(c[ch].reference_spectrum,mode==2?256:2048,value(0x3f70a3d7u));
            scale(core[ch].spectrum,2048,value(0x3f70a3d7u));
        }
    }
    if(cfg->sample_rate==44100&&channels==2&&units<32){
        uint32_t start=2048;
        if(profile==13)start=1208;
        else if(profile==15)start=1394;
        for(uint32_t ch=0;ch<channels;++ch)for(uint32_t k=start;k<2048;++k){
            c[ch].reference_spectrum[k]=0.0f;core[ch].spectrum[k]=0.0f;
        }
    }
    if(channels==2)for(uint32_t band=0;band<cfg->subbands;++band)if(cfg->source_swap[band]==1){
        exchange(core[0].spectrum+band*128,core[1].spectrum+band*128);
        if(mode!=2||band<2)exchange(c[0].reference_spectrum+band*128,c[1].reference_spectrum+band*128);
    }
    for(uint32_t ch=0;ch<channels;++ch){
        plus_spectrum_low_ratios(c[ch].reference_spectrum,cfg->backend,c[ch].statistics);
        c[ch].boundary=c[ch].statistics[0]>4.0f?2:1;
        if(c[ch].activity==0)++c[ch].boundary;
        if(c[ch].statistics[1]>8.0f)c[ch].boundary+=2;
        else if(c[ch].statistics[1]>4.0f)++c[ch].boundary;
        if(c[ch].boundary<2&&c[ch].statistics[1]>1.0f)c[ch].boundary=2;
    }
    if(channels==2){
        const int32_t difference=(int32_t)c[0].boundary-(int32_t)c[1].boundary;
        if(difference==1||difference== -1){
            const int16_t maximum=c[0].boundary>c[1].boundary?c[0].boundary:c[1].boundary;
            c[0].boundary=maximum;c[1].boundary=maximum;
        }
    }
    for(uint32_t ch=0;ch<channels;++ch){
        if(mode==2){
            float temporary[32];
            plus_feature_coarse(c[ch].reference_spectrum,units<12?units:12,
                c[ch].neutral_coarse,temporary,cfg->backend);
        }else plus_feature_coarse(c[ch].reference_spectrum,units,
            c[ch].neutral_coarse,c[ch].reference_peak,cfg->backend);
        plus_feature_coarse(core[ch].spectrum,units,core[ch].coarse,core[ch].numerator,cfg->backend);
        plus_spectrum_peakiness(core[ch].spectrum,core[ch].numerator,units,
            cfg->backend,c[ch].current_peakiness);
        if(mode==2)for(uint32_t q=12;q<units;++q)c[ch].neutral_coarse[q]=core[ch].coarse[q];
    }
    uint32_t average_units=coherence_unit_boundary[cfg->coherence_start];
    if(average_units>units)average_units=units;
    for(uint32_t ch=0;ch<channels;++ch){
        for(uint32_t q=0;q<units;++q){
            c[ch].significant[q]=(c[ch].neutral_coarse[q]+c[ch].previous_neutral_coarse[q]+1)/2;
            c[ch].average_peakiness[q]=(float)(((double)c[ch].current_peakiness[q]+(double)c[ch].previous_peakiness[q])*0.5);
        }
        float total=0.0f;
        for(uint32_t q=0;q<average_units;++q)total+=c[ch].average_peakiness[q];
        c[ch].average_tonality=average_units?(float)((double)total/(double)average_units):0.0f;
        if(mode==2){
            if(c[ch].maximum_word_length<7&&c[ch].average_tonality>4.0f)++c[ch].maximum_word_length;
            if(((channels==2&&profile<14)||(channels==1&&profile<12))&&
                c[ch].maximum_word_length<7&&c[ch].average_tonality>6.0f)c[ch].maximum_word_length=7;
            if(cfg->feature_flags)c[ch].maximum_word_length=7;
        }
    }
    int32_t maximum=1;
    for(uint32_t ch=0;ch<channels;++ch)for(uint32_t q=0;q<units;++q)
        if(c[ch].significant[q]>maximum)maximum=c[ch].significant[q];
    for(uint32_t ch=0;ch<channels;++ch)
        c[ch].demand_scale=(float)((double)(c[ch].maximum_word_length*10)/(double)maximum);
    if(mode!=2)for(uint32_t ch=0;ch<channels;++ch){
        for(uint32_t q=0;q<32;++q)c[ch].unit_limits[q]=c[ch].maximum_word_length;
        for(uint32_t q=0;q<units;++q){
            if(c[ch].average_peakiness[q]>=6.0f)c[ch].unit_limits[q]+=2;
            else if(c[ch].average_peakiness[q]>=3.5f)++c[ch].unit_limits[q];
        }
        if((channels==2&&profile>10&&profile<20)||(channels==1&&profile==9)){
            c[ch].gain_flag_word&=0xffffff00u;
            const plus_given_gain_band* first=&c[ch].gain.bands[0];
            for(uint32_t p=0;p<first->points;++p)if(first->levels[p]<5||first->levels[p]>7){
                c[ch].gain_flag_word|=1;
                for(uint32_t q=0;q<8;++q)++c[ch].unit_limits[q];
            }
        }
        for(uint32_t q=0;q<8;++q)if(c[ch].unit_limits[q]<c[ch].boundary)c[ch].unit_limits[q]=c[ch].boundary;
        for(uint32_t q=0;q<units;++q)if(c[ch].unit_limits[q]>7)c[ch].unit_limits[q]=7;
        if(cfg->feature_flags&0x7cu)for(uint32_t q=0;q<units;++q)c[ch].unit_limits[q]=7;
    }
    return 0;
}
