#include "recovered_plus_initial.h"
#include <string.h>
#include "initial_tables.inc"
static float number(uint32_t u){float x;memcpy(&x,&u,4);return x;}
static double number64(const uint32_t *u){double x;memcpy(&x,u,8);return x;}
static double add(double a,double b){volatile double x=a+b;return x;}
static double mul(double a,double b){volatile double x=a*b;return x;}
static float stored(double x){volatile float y=(float)x;return y;}
static double absolute(double x){return x<0?-x:x;}
static int32_t absolute_int(int32_t x){return x<0?-x:x;}
static double maximum(double a,double b){return a>=b?a:b;}
int sony_recovered_plus_initial_stereo_prepare(const sony_plus_initial_config *g,
    const sony_plus_initial_channel channels[2],sony_plus_initial_stereo *s,
    sony_plus_initial_result *r){
    if(!g||!channels||!s||!r||g->channels!=2||g->maximum_units>32||g->maximum_subbands>16||s->analyzed_bands<0||s->analyzed_bands>16)return -1;
    for(uint32_t q=0;q<g->maximum_units;++q)if(s->correlation[q]<number(plus_initial_45662c[0]))r->stereo_flags[q]=1;
    if(g->request_mode==2)return 0;
    const uint32_t row=g->profile>=27?0:g->profile>=19?1:g->profile>=13?2:3;
    const uint32_t bands=(uint32_t)s->analyzed_bands;int32_t desired[32]={0};
    for(uint32_t b=1;b<bands;++b)if(s->band_correlation[b]>number(plus_initial_41cc20[16*row+b])){
        for(uint32_t q=plus_initial_41a748[b];q<plus_initial_41a748[b+1];++q)
            if(s->coarse[0][q]==s->coarse[1][q]&&s->base[q]<=s->coarse[0][q])r->stereo_flags[q]=1;
    }
    for(uint32_t b=0;b<bands;++b){
        const int32_t penalty=(int32_t)mul(s->band_correlation[b],number(plus_initial_4565c0[0]));
        for(uint32_t q=plus_initial_41a748[b];q<plus_initial_41a748[b+1];++q){
            const int32_t peak_penalty=(int32_t)mul(maximum(channels[0].peakiness[q],channels[1].peakiness[q]),number(plus_initial_456698[0]));
            int32_t value=(int32_t)plus_initial_41cd20[32*row+q]-peak_penalty-penalty;if(value<3)value=3;desired[q]=value;
        }
    }
    for(uint32_t b=0;b<bands;++b)if(s->band_flags[b]){
        for(uint32_t q=plus_initial_41a748[b];q<plus_initial_41a748[b+1];++q)
            if(!r->stereo_flags[q]&&(int32_t)maximum(s->coarse[0][q],s->coarse[1][q])-s->base[q]>=desired[q])r->stereo_flags[q]=1;
    }
    for(uint32_t b=0;b<bands;++b)if(s->band_correlation[b]<number(plus_initial_456694[0])){
        for(uint32_t q=plus_initial_41a748[b];q<plus_initial_41a748[b+1];++q)
            if((int32_t)maximum(channels[0].peakiness[q],channels[1].peakiness[q])>6)r->stereo_flags[q]=0;
    }
    for(uint32_t b=0;b<g->maximum_subbands;++b){
        const uint32_t begin=plus_initial_41a748[b],end=plus_initial_41a748[b+1];
        int32_t low=absolute_int(s->coarse[0][begin]-s->coarse[1][begin]),high=low;
        for(uint32_t q=begin+1;q<end;++q){const int32_t value=absolute_int(s->coarse[0][q]-s->coarse[1][q]);if(low>value)low=value;if(high<value)high=value;}
        s->band_output[b]=0;
        if(s->band_correlation[b]<number(plus_initial_45660c[0])&&high<=1&&
           (s->band_score[b]<number(plus_initial_45660c[0])||high==low)){
            s->band_output[b]=1;
            for(uint32_t q=begin;q<end;++q){
                const int32_t similar=absolute((double)s->channel_metric[0][q]-s->channel_metric[1][q])<number64(plus_initial_4565e8);
                if((double)similar!=number(plus_initial_4565e0[0]))r->stereo_flags[q]=1;
                for(uint32_t i=plus_initial_41a6e0[q];i<plus_initial_41a6e0[q+1];++i)s->spectrum[1][i]=-s->spectrum[1][i];
            }
        }
    }
    for(uint32_t b=0;b<g->maximum_subbands;++b)if(s->band_output[b]||s->band_correlation[b]>=number(plus_initial_456608[0])){
        for(uint32_t q=plus_initial_41a748[b];q<plus_initial_41a748[b+1];++q)if(r->stereo_flags[q]){
            if(s->coarse[0][q]==s->coarse[1][q]+1)s->coarse[0][q]=s->coarse[1][q];
            else if(s->coarse[1][q]==s->coarse[0][q]+1)s->coarse[1][q]=s->coarse[0][q];
        }
    }
    if(s->context_policy==1){
        const float limit=number(g->profile>=25?plus_initial_456690[0]:g->profile>=23?plus_initial_45668c[0]:g->profile>=19?plus_initial_456688[0]:plus_initial_456684[0]);
        for(uint32_t b=0;b<bands;++b)if(!s->band_output[b]&&limit>s->band_correlation[b])
            for(uint32_t q=plus_initial_41a748[b];q<plus_initial_41a748[b+1];++q)if(r->stereo_flags[q]&&s->coarse[0][q]==s->coarse[1][q])r->stereo_flags[q]=0;
    }
    for(uint32_t b=bands;b<g->maximum_subbands;++b)if(s->band_score[b]>number(plus_initial_45660c[0]))
        for(uint32_t q=plus_initial_41a748[b];q<plus_initial_41a748[b+1];++q)r->stereo_flags[q]=1;
    return 0;
}
int sony_recovered_plus_initial_round(const float *scores,int32_t *out,
    int32_t maximum,int32_t count){
    if(count<=0)return 0;if(!scores||!out||count>32||maximum<1||maximum>7)return -1;
    for(int32_t i=0;i<count;++i){int32_t value=(int32_t)add(scores[i],number(plus_initial_4565a8[0]));if(value>maximum)value=maximum;else if(value<1)value=1;out[i]=value;}return 0;
}
static uint32_t curve_index(uint32_t channels,uint32_t profile){
    uint32_t row=0;
    if(channels==2){if(profile>=13)row=1;if(profile>=15)row=2;if(profile>=19)row=3;if(profile>=23)row=4;if(profile>=24)row=5;if(profile>=25)row=6;if(profile>=27)row=7;if(profile>=29)row=8;}
    else{if(profile>=11)row=1;if(profile>=13)row=2;if(profile>=15)row=3;if(profile>=19)row=4;if(profile>=23)row=7;}
    return row;
}
int sony_recovered_plus_initial_words(const sony_plus_initial_config *g,
    sony_plus_initial_channel channels[2],sony_plus_initial_stereo *stereo,
    sony_plus_initial_result *r){
    if(!g||!channels||!r||!stereo||g->channels<1||g->channels>2||g->maximum_units<1||g->maximum_units>32||g->maximum_subbands<1||g->maximum_subbands>16||g->profile>31||g->request_mode<0||g->request_mode>3)return -1;
    const uint32_t count=g->maximum_units;const float epsilon=number(plus_initial_45662c[0]);
    for(uint32_t i=0;i<32;++i){r->stereo_flags[i]=0;r->swap_flags[i]=0;}
    if(g->channels==2&&sony_recovered_plus_initial_stereo_prepare(g,channels,stereo,r))return -1;
    r->format_flag=g->profile<27;
    const uint32_t row=curve_index(g->channels,g->profile);
    for(uint32_t ch=0;ch<g->channels;++ch){
        sony_plus_initial_channel *c=channels+ch;
        if(c->maximum_word_length<1||c->maximum_word_length>7)return -1;
        if(g->special_mode){const float factor=stored(mul(c->scale,number(plus_initial_456660[0])));for(uint32_t q=0;q<count;++q)c->demand[q]=stored(mul(c->significant[q],factor));}
        else{
            for(uint32_t q=0;q<count;++q)c->demand[q]=stored(mul(mul(c->significant[q],c->scale),number(plus_initial_41ba68[32*row+q])));
            if(g->sample_rate==48000)for(uint32_t q=18;q<count&&q<31;++q)c->demand[q]=stored(mul(mul(c->significant[q],c->scale),number(plus_initial_41ba68[32*row+q+1])));
            double activity=number(plus_initial_4565c0[0]);
            if(c->activity>2)activity=add(mul(c->activity,number(plus_initial_4565c0[0])),number(plus_initial_4565a8[0]));
            else if(c->activity==2)activity=number(plus_initial_456600[0]);
            for(uint32_t q=4;q<8;++q)c->demand[q]=stored(add(activity,c->demand[q]));
            const uint32_t threshold=g->channels==2?27:23;
            if(g->profile>=threshold){for(uint32_t q=0;q<24;++q)c->demand[q]=stored(add(c->demand[q],number(plus_initial_456600[0])));for(uint32_t q=28;q<count;++q)c->demand[q]=stored(add(c->demand[q],-number(plus_initial_456600[0])));}
            if(!c->flag){
                const uint32_t low=g->channels==2?15:13,high=g->channels==2?19:15;
                float boost=0;if(g->profile<=low)boost=number(plus_initial_4565f0[0]);else if(g->profile<=high)boost=number(plus_initial_4565a8[0]);
                if(boost)for(uint32_t q=6;q<8;++q)c->demand[q]=stored(add(c->demand[q],boost));
            }
            for(uint32_t q=0;q<count;++q){float boost=0;if(c->peakiness[q]>=number(plus_initial_4565d4[0]))boost=number(plus_initial_45665c[0]);else if(c->peakiness[q]>=number(plus_initial_45661c[0]))boost=number(plus_initial_4565f0[0]);else if(c->peakiness[q]>=number(plus_initial_456618[0]))boost=number(plus_initial_4565a8[0]);if(boost)c->demand[q]=stored(add(c->demand[q],boost));}
        }
        if(sony_recovered_plus_initial_round(c->demand,(int32_t*)c->word_length,c->maximum_word_length,(int32_t)count))return -1;
        if((g->channels==1&&g->profile<23)||(g->channels==2&&g->profile<27)){
            for(uint32_t q=0;q<count;++q)if(!c->significant[q])c->word_length[q]=0;
        }else{
            for(uint32_t band=0;band<g->maximum_subbands;++band){
                if(stereo->energy_left[band]>=epsilon&&(g->channels!=2||stereo->energy_right[band]>=epsilon))continue;
                for(uint32_t q=plus_initial_41a748[band];q<plus_initial_41a748[band+1];++q)if(!c->significant[q])c->word_length[q]=0;
            }
        }
        if(g->channels==2&&ch==1)for(uint32_t q=0;q<count;++q){
            if(!c->word_length[q]){if(channels[0].word_length[q])r->swap_flags[q]=1;r->stereo_flags[q]=0;}
            else if(!channels[0].word_length[q])r->stereo_flags[q]=0;
            else if(r->stereo_flags[q])c->word_length[q]=0;
        }
        for(uint32_t q=0;q<32;++q)c->history[q]=(int32_t)c->word_length[q];
    }
    if(count>=29&&count<=31){for(uint32_t ch=0;ch<g->channels;++ch)for(uint32_t q=count;q<32;++q)channels[ch].word_length[q]=0;r->limit_units=32;r->limit_subbands=16;}
    else{r->limit_units=count;r->limit_subbands=g->maximum_subbands;}
    for(uint32_t ch=0;ch<g->channels;++ch)channels[ch].noise_flag=channels[ch].noise>number(plus_initial_456610[0]);
    uint32_t active=count;while(active){uint32_t any=channels[0].word_length[active-1];if(g->channels==2)any|=channels[1].word_length[active-1];if(any)break;--active;}
    r->active_units=active;r->active_subbands=(uint32_t)plus_initial_41a724[active]+1;
    return 0;
}
