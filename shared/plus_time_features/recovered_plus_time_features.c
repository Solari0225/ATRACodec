#include "recovered_plus_time_features.h"
#include "recovered_plus_time_detection.h"
#include "recovered_plus_time_fft.h"
#include "recovered_plus_time_tonal.h"
#include "../plus_analysis/independent_plus_math.h"
static float spill(double x){volatile float value=(float)x;return value;}
double sony_plus_time_log_measure(float value){
    return value>0?sony_plus_math_positive_ln(value)*8.68588924407959:-160.;
}
int sony_plus_time_coherence(float *const left[16],float *const right[16],uint32_t count,uint32_t bands,float contrast[16],float left_energy[16],float right_energy[16],uint32_t backend){
    if(!left||!right||!contrast||!left_energy||!right_energy||count>256||bands>16)return -1;
    for(uint32_t b=0;b<bands;++b){float difference[256],l,r,d,ratio;
        sony_plus_time_mix(left[b],right[b],difference,count,0,backend);
        sony_plus_time_triple_dot(left[b],left[b],right[b],right[b],difference,difference,&l,&r,&d,count,backend);
        if((l==0&&r==0)||d==0)ratio=0.001f;else if(l==0||r==0)ratio=1.f;else ratio=spill((double)d/(l>r?l:r));
        double value=-sony_plus_time_log_measure(ratio);if(value>60)value=60;contrast[b]=spill(value);left_energy[b]=l;right_energy[b]=r;
    }return 0;
}
int sony_plus_time_detect(sony_plus_time_common *current,float *const input[32],int32_t order[16],float energy[2][16],uint32_t bands,uint32_t channels,uint32_t *flags,uint32_t backend){
    if(!current||!input||!order||!energy||!flags||bands<2||bands>16||!channels||channels>2)return -1;
    float total_channel[2]={0,0},sorted[16];int32_t ranked[2][16];
    for(uint32_t ch=0;ch<channels;++ch){
        for(uint32_t b=0;b<bands;++b){if(!input[ch*16+b])return -1;const float *s=input[ch*16+b]+128;energy[ch][b]=spill(sony_plus_time_dot(s,s,128,backend)*.0078125);total_channel[ch]=spill((double)total_channel[ch]+energy[ch][b]);sorted[b]=energy[ch][b];}
        sony_plus_time_sort_power(sorted,ranked[ch],bands);
        if(total_channel[ch]>0){const double top=(double)energy[ch][(uint32_t)ranked[ch][0]]+energy[ch][(uint32_t)ranked[ch][1]];
            if((double)total_channel[ch]*0.9999899864196777>=top){current->amplitude_mode=0;current->bands=1;return 3;}}
    }
    if(channels==2){float contrast[16],left[16],right[16];sony_plus_time_coherence(input,input+16,256,bands,contrast,left,right,backend);
        for(uint32_t b=0;b<bands;++b){
            if(contrast[b]>=20)current->flags[0][b+2]=1;
            else if(contrast[b]<-11){current->flags[0][b+2]=1;current->flags[1][b+2]=1;}
            else{current->flags[0][b+2]=0;current->flags[1][b+2]=0;}
        }
    }else for(uint32_t b=0;b<bands;++b)current->flags[0][b+2]=current->flags[1][b+2]=0;
    int32_t status[2]={3,3},eligible[2]={0,0},computed[2][16]={{0}},peak[2][16];float metric[2][16]={{0}};
    const float threshold=(*flags&2u)?2.4000000953674316f:12.f;const int32_t peak_gap=(*flags&2u)?8:4;
    for(uint32_t ch=0;ch<channels;++ch){
        if(total_channel[ch]<1){eligible[ch]=0;status[ch]=0;continue;}eligible[ch]=1;
        for(uint32_t k=0;k<2&&eligible[ch];++k){const uint32_t b=(uint32_t)ranked[ch][k];
            if(!computed[ch][b]){float power[65];
                if(current->flags[0][b+2]){float mixed[128];sony_plus_time_mix(input[b]+128,input[16+b]+128,mixed,128,current->flags[1][b+2]?2:1,backend);sony_plus_time_fft_power(mixed,1,128,power);
                    const int32_t p=sony_plus_time_largest_positive(power,65);const float ratio=spill(sony_plus_time_peak_ratio(power,65));
                    for(uint32_t j=0;j<2;++j){peak[j][b]=p;metric[j][b]=ratio;computed[j][b]=1;}
                }else{sony_plus_time_fft_power(input[ch*16+b]+128,1,128,power);peak[ch][b]=sony_plus_time_largest_positive(power,65);metric[ch][b]=spill(sony_plus_time_peak_ratio(power,65));}
            }
            if(!(metric[ch][b]>threshold)){eligible[ch]=0;break;}
            if(!(*flags&2u)){int32_t delta=peak[ch][b]-peak[ch][(uint32_t)ranked[ch][0]];if(delta<0)delta=-delta;if(delta>peak_gap)eligible[ch]=0;}
        }
        if(eligible[ch])status[ch]=1;
    }
    float total_band[16]={0};
    for(uint32_t ch=0;ch<channels;++ch)for(uint32_t b=0;b<bands;++b){const float *s=input[ch*16+b];energy[ch][b]=spill(sony_plus_time_dot(s,s,256,backend)*.00390625);total_band[b]=spill((double)total_band[b]+energy[ch][b]);}
    for(uint32_t b=0;b<bands;++b)sorted[b]=total_band[b];sony_plus_time_sort_power(sorted,order,bands);
    int32_t combined=0;for(uint32_t ch=0;ch<channels;++ch)combined|=status[ch];if(combined==1)*flags|=1;
    if((*flags&3u)==3u){current->amplitude_mode=1;current->bands=bands;return 1;}
    current->amplitude_mode=0;current->bands=bands>1?1:bands;return 3;
}
int sony_plus_time_budget(const sony_plus_time_common *current,const float energy[2][16],uint32_t selection_mode,int32_t counts[2][16],const int32_t order[16],int32_t profile,uint32_t channels){
    if(!current||!energy||!counts||!order||current->bands>16||selection_mode>2||!channels||channels>2)return -1;
    const uint32_t bands=current->bands;const int32_t maximum=profile<11?0:profile<13?12:48;float total[16]={0},weight[16],sum=0;
    for(uint32_t b=0;b<bands;++b){double e=0;for(uint32_t ch=0;ch<channels;++ch)e+=energy[ch][b];total[b]=spill(e);weight[b]=total[b]>=1?spill(sony_plus_time_log_measure(total[b])):0;sum=spill((double)sum+weight[b]);}
    if(!(sum>0)){for(uint32_t ch=0;ch<channels;++ch)for(uint32_t b=0;b<bands;++b)counts[ch][b]=0;return 0;}
    const float inv=spill(1.0/sum);int32_t allocation[16];
    for(uint32_t b=0;b<bands;++b){const double x=((double)inv*weight[b])*(float)maximum+.5;int32_t n=(int32_t)x;if((double)n>x)--n;allocation[b]=n<1?1:n;}
    int32_t remaining=maximum;
    for(uint32_t ordinal=0;ordinal<bands;++ordinal){const int32_t at=order[ordinal];if(at<0||at>=(int32_t)bands)return -1;const uint32_t b=(uint32_t)at;
        int32_t n=allocation[b];if(n>remaining)n=remaining;
        if(channels==1)counts[0][b]=n;
        else if(current->flags[0][b+2]){counts[0][b]=n;counts[1][b]=0;}
        else{counts[0][b]=n-n/2;counts[1][b]=n-counts[0][b];}
        if(selection_mode==1){const uint32_t top=profile<15?3:16;const int32_t cap=profile<13?3:profile<15?8:15,low=profile<15?0:15;
            for(uint32_t ch=0;ch<channels;++ch){int32_t *v=counts[ch]+b;
                if(ordinal<top){if(*v>1){if(*v>cap)*v=cap;}else if(*v>low)*v=low;}
                else if(*v>low)*v=low;
            }
        }else if(selection_mode==2)for(uint32_t ch=0;ch<channels;++ch)if(counts[ch][b]>3)counts[ch][b]=3;
        for(uint32_t ch=0;ch<channels;++ch)remaining-=counts[ch][b];
    }return 0;
}
int sony_plus_time_features_initialize(sony_plus_time_frame *frame){
    if(!frame)return -1;unsigned char *p=(unsigned char*)frame;for(uint32_t i=0;i<(uint32_t)sizeof(*frame);++i)p[i]=0;
    for(uint32_t ch=0;ch<2;++ch)sony_plus_time_clear(&frame->common,frame->bands[ch],16);return 0;
}
int sony_plus_time_features_process(const sony_plus_time_feature_config *config,const sony_plus_time_frame *previous,sony_plus_time_frame *current,float *const input[32],uint32_t *flags,int32_t *error){
    if(!config||!previous||!current||previous==current||!input||!flags||!error||config->bands<2||config->bands>16||config->channels<1||config->channels>2)return -1;
    const uint32_t channels=(uint32_t)config->channels,bands=(uint32_t)config->bands,backend=config->backend!=0;
    for(uint32_t ch=0;ch<channels;++ch)sony_plus_time_clear(&current->common,current->bands[ch],bands);
    float energy[2][16]={{0}};int32_t order[16]={0},counts[2][16]={{0}};
    const int detected=sony_plus_time_detect(&current->common,input,order,energy,bands,channels,flags,backend);
    if(detected<0)return detected;
    const int selected=detected==1&&config->profile_id>=(channels==2?13:11);
    if(selected){
        if(sony_plus_time_budget(&current->common,energy,1,counts,order,config->profile_id,channels))return -1;
        uint32_t used=0;
        for(uint32_t ordinal=0;ordinal<current->common.bands;++ordinal){const uint32_t b=(uint32_t)order[ordinal];
            if(current->common.flags[0][b+2]){
                if(counts[0][b]>0){float mixed[256],power[129];sony_plus_time_mix(input[b],input[16+b],mixed,256,current->common.flags[1][b+2]?2:1,backend);sony_plus_time_fft_power256(mixed,256,power,backend);const int32_t peak=sony_plus_time_largest_positive(power,129);
                    sony_plus_time_band *out=&current->bands[0][b];out->first_note=used;if(sony_plus_time_extract(mixed,&current->common,out,current->common.amplitude_mode,peak,(uint32_t)counts[0][b],backend))return -1;current->bands[1][b]=*out;used+=out->count;}
            }else for(uint32_t ch=0;ch<channels;++ch)if(counts[ch][b]>0){float power[129];sony_plus_time_fft_power256(input[ch*16+b],256,power,backend);const int32_t peak=sony_plus_time_largest_positive(power,129);
                sony_plus_time_band *out=&current->bands[ch][b];out->first_note=used;if(sony_plus_time_extract(input[ch*16+b],&current->common,out,current->common.amplitude_mode,peak,(uint32_t)counts[ch][b],backend))return -1;used+=out->count;}
        }
        uint32_t unique=0;for(uint32_t ch=0;ch<channels;++ch)for(uint32_t b=0;b<bands;++b)if(ch==0||!current->common.flags[0][b+2])unique+=current->bands[ch][b].count;
        if(unique>48){for(uint32_t ch=0;ch<channels;++ch)sony_plus_time_clear(&current->common,current->bands[ch],bands);*error=0x8f;}
    }
    for(uint32_t b=0;b<bands;++b)for(uint32_t ch=0;ch<channels;++ch){
        const sony_plus_time_band *prior=&previous->bands[ch][b],*now=&current->bands[ch][b];
        if(prior->count||(selected&&now->count)){float signal[128];if(sony_plus_time_overlap(&previous->common,prior,previous->common.flags[1][b+2],&current->common,now,current->common.flags[1][b+2],signal,ch,backend))return -1;sony_plus_time_mix(input[ch*16+b],signal,input[ch*16+b],128,0,backend);}
    }
    current->common.present=(uint32_t)selected;return 0;
}
