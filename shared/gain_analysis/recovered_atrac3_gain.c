#include "recovered_atrac3_gain.h"
#include <float.h>
_Static_assert(sizeof(float)==4&&FLT_MANT_DIG==24&&sizeof(double)==8&&DBL_MANT_DIG==53,
               "IEEE binary32/binary64 required");
#include "gain_tables.inc"
static float number(uint32_t bits){union{uint32_t bits;float value;}d;d.bits=bits;return d.value;}
static uint32_t word(float value){union{uint32_t bits;float value;}d;d.value=value;return d.bits;}
static float stored_product(double a,double b){volatile double value=a*b;return(float)value;}
static int32_t ratio_steps(float value,double reference)
{
    volatile double ratio=(double)value/reference;
    const float encoded=stored_product(ratio,(double)number(gain_scalar_words[2]));
    return (int32_t)(word(encoded)>>23)-127;
}
static int32_t select_points(sony_atrac3_gain_record *r,const float envelope[64],int32_t mode)
{
    const float preceding=r->last_quarter_maximum;
    float maxima[8];
    for(uint32_t block=0;block<8;++block){
        float maximum=envelope[block*4];
        for(uint32_t j=1;j<4;++j)if(maximum<envelope[block*4+j])maximum=envelope[block*4+j];
        maxima[block]=maximum;
    }
    r->last_quarter_maximum=maxima[7];
    double reference=(double)maxima[7];
    const int32_t limit=mode>0?(8-mode)*8:64;
    for(int32_t i=32;i<limit;++i)if(reference<(double)envelope[i])reference=(double)envelope[i];
    if(reference<(double)number(gain_scalar_words[5]))reference=(double)number(gain_scalar_words[5]);
    float threshold=stored_product(reference,(double)number(gain_scalar_words[4]));
    int32_t down_budget=4,tail=7;
    r->location[7]=32;
    for(int32_t block=8;block>=0;--block){
        const float maximum=block?maxima[block-1]:preceding;
        if((double)maximum<reference)continue;
        if(maximum>number(gain_scalar_words[3])&&maximum>threshold){
            uint32_t location=(uint32_t)(block*4);
            if(block&&envelope[block*4]<threshold&&envelope[block*4-1]<threshold)
                location=(uint32_t)(block*4-1-(envelope[block*4-2]<threshold));
            int32_t step=ratio_steps(maximum,reference);
            --tail;r->location[tail]=location;
            if(step>down_budget)step=down_budget;
            down_budget-=step;r->level[tail]=-step;
            if(down_budget<=0||tail==5)break;
        }
        reference=(double)maximum;
        threshold=stored_product(reference,(double)number(gain_scalar_words[4]));
    }
    const float budget_value=stored_product((double)r->history_maximum,
                                           (double)number(gain_scalar_words[1]));
    int32_t up_budget=131-(int32_t)(word(budget_value)>>23);
    if(up_budget>15)up_budget=15;
    up_budget-=down_budget;
    int32_t count=0;
    if(up_budget>0){
        const float factor=number(mode==-1?0x40000000u:0x3fcccccdu);
        reference=(double)r->history_maximum;
        if(reference<(double)envelope[0])reference=(double)envelope[0];
        volatile double boundary=(double)factor*reference;
        for(uint32_t position=0;position<r->location[tail];++position){
            const float value=envelope[position+1];
            if((double)value<reference)continue;
            if(value>number(gain_scalar_words[3])&&(double)value>boundary){
                int32_t step=ratio_steps(value,reference);
                r->location[count]=position;
                if(count>=1&&r->location[count-1]==position-1&&step>=r->level[count-1]){
                    --count;r->location[count]=position;
                    up_budget+=r->level[count];step+=r->level[count];
                }
                if(step>up_budget)step=up_budget;
                up_budget-=step;r->level[count++]=step;
                if(count==tail||up_budget<=0)break;
            }
            reference=(double)value;boundary=(double)factor*reference;
        }
    }
    for(;tail<7;++tail){r->level[count]=r->level[tail];r->location[count]=r->location[tail];++count;}
    int32_t level=4;
    for(int32_t i=count;i-->0;){r->level[i]+=level;level=r->level[i];}
    return count;
}
int32_t sony_recovered_atrac3_gain_band(sony_atrac3_gain_channel *channel,
    uint32_t band,const sony_atrac3_gain_config *config,const float peaks[32])
{
    if(!channel||!config||!peaks||band>3)return -1;
    sony_atrac3_gain_record *r=channel->band+band;
    if(r->point_count>7||channel->band[1].point_count>7)return -1;
    float envelope[64];uint32_t maximum=0x508b771fu;
    for(uint32_t i=0;i<32;++i){
        const uint32_t bits=word(r->envelope_history[i]);if(maximum<bits)maximum=bits;
        envelope[i]=r->envelope_history[i];envelope[i+32]=peaks[i];r->envelope_history[i]=peaks[i];
    }
    int32_t mode=(int32_t)band;
    if(config->extended_header){
        mode=-1;
        if(config->current[band]!=config->previous[band])mode=5;
        else if(config->current[band]!=3)mode=0;
    }
    int32_t count=select_points(r,envelope,mode);r->history_maximum=number(maximum);
    if(!band)channel->previous_low_band_count=r->point_count;
    if(!count){
        if(band||config->extended_header||r->point_count||!channel->band[1].point_count||
           r->history_maximum>number(gain_scalar_words[0]))return 0;
        int32_t minimum=4,largest=4;
        for(uint32_t i=0;i<channel->band[1].point_count;++i){
            const int32_t level=channel->band[1].level[i];
            if(minimum>level)minimum=level;if(largest<level)largest=level;
        }
        if(largest-minimum<2)return 0;
        const uint32_t location=channel->band[1].location[0];
        for(uint32_t i=0;i<=location;++i)if(envelope[i]>number(gain_scalar_words[0]))return 0;
        count=1;r->location[0]=location;r->level[0]=5;
    }
    if(count<0||count>7)return -2;
    r->level[count]=4;
    uint32_t position=0;
    for(int32_t point=0;;++point){
        const int32_t old=r->level[point];
        if(old<0||old>15)return -2;
        if(old==4&&point==count)return count;
        if(point>=count)return -2;
        const uint32_t endpoint=r->location[point]*8;
        if(endpoint>248||endpoint<position)return -2;
        if(old!=4){
            const float scale=number(gain_scale_words[old]);
            for(;position<=endpoint;++position)
                channel->previous_samples[4*position+band]=stored_product(
                    (double)scale,(double)channel->previous_samples[4*position+band]);
        }else position=endpoint+1;
        const int32_t difference=r->level[point+1]-old;
        for(int32_t step=1;step<8;++step){
            const uint32_t phase=(uint32_t)(old*8+step*difference);
            const float scale=number((phase<<20)+gain_fraction_words[phase&7]);
            if(position>=256)return -2;
            channel->previous_samples[4*position+band]=stored_product(
                (double)scale,(double)channel->previous_samples[4*position+band]);
            ++position;
        }
    }
}
int sony_recovered_atrac3_gain_frame(sony_atrac3_gain_channel *channel,
    const sony_atrac3_gain_config *config,const float peaks[4][32])
{
    if(!channel||!config||!peaks)return -1;
    for(uint32_t i=4;i-->0;){
        const int32_t count=sony_recovered_atrac3_gain_band(channel,i,config,peaks[i]);
        if(count<0)return count;
        channel->band[i].point_count=(uint32_t)count;channel->band[i].level[count]=4;
    }
    return 0;
}
