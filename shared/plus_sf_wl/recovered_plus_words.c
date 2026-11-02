#include "recovered_plus_words.h"
#include "../plus_quantization/recovered_plus_quantization.h"
#include "../plus_allocation/recovered_plus_initial.h"
#include "sf_wl_math.inc"
typedef union words_number {uint32_t u;float f;} words_number;
static float number(uint32_t u){words_number v;v.u=u;return v.f;}
static double add(double a,double b){volatile double c=a+b;return c;}
static double mul(double a,double b){volatile double c=a*b;return c;}
static float stored(double a){volatile float b=(float)a;return b;}
static int16_t word(int32_t a){return (int16_t)(uint16_t)a;}
static float constant(uint32_t offset){return number(sf_wl_numeric_constants[(offset-0x4565a0u)/4u]);}
static int validate(const sony_plus_word_config *g,const sony_plus_word_channel *c,const sony_plus_word_shared *s){
    if(!g||!c||!s||g->channels<1||g->channels>2||g->units>32||g->profile>31)return -1;
    for(uint32_t ch=0;ch<g->channels;++ch)if(c[ch].input.bank>1||c[ch].maximum_word_length<1||c[ch].maximum_word_length>7)return -1;
    return 0;
}
static void update_total(const sony_plus_word_config *g,sony_plus_word_shared *s){
    int32_t n=s->overhead;for(uint32_t ch=0;ch<g->channels;++ch)n+=s->channel_cost[ch];s->total=word(n);
}
int sony_recovered_plus_unit_costs(const sony_plus_unit_cost_input *in,const int32_t *wl,
    const int32_t *changed,uint32_t n,sony_plus_cost_matrix *matrix,int16_t *total){
    if(!in||!wl||!matrix||!total||in->bank>1||n>32)return -1;
    int32_t sum=0;
    for(uint32_t q=0;q<n;++q){
        if(wl[q]<=0){matrix->book[q]=0;continue;}
        if(wl[q]>7)return -1;
        if(!changed||changed[q]){
            if(sony_recovered_plus_quantization_cost(in->bank,(uint32_t)wl[q],(uint32_t)in->threshold_index[q],
                in->threshold_factor[q],sf_wl_qu_count[q],in->normalized+sf_wl_qu_start[q],in->simd,matrix->costs[q]))return -1;
            uint32_t best=0;for(uint32_t j=1;j<4;++j)if(matrix->costs[q][j]<matrix->costs[q][best])best=j;matrix->book[q]=(int32_t)best;
        }
        if(matrix->book[q]<0||matrix->book[q]>3)return -1;
        sum+=matrix->costs[q][matrix->book[q]];
    }
    *total=word(sum);return 0;
}
int sony_recovered_plus_zero_unit(uint32_t bank,uint32_t unit,uint32_t wl,
    sony_plus_cost_matrix *matrix,int32_t *saved){
    if(!matrix||!saved||bank>1||unit>31||wl<1||wl>7||matrix->book[unit]<0||matrix->book[unit]>3)return -1;
    const uint32_t index=7u*bank+wl,book=sf_wl_zero_unit_book[index];if(book>3)return -1;
    const int32_t cost=(int32_t)sf_wl_zero_unit_cost[index]*(sf_wl_qu_count[unit]>>4);
    *saved=matrix->costs[unit][matrix->book[unit]]-cost;matrix->book[unit]=(int32_t)book;matrix->costs[unit][book]=word(cost);return 0;
}
static uint32_t threshold_row(uint32_t ch,uint32_t p){
    if(ch==2){if(p<=9)return 0;if(p<=11)return 1;if(p<=13)return 2;if(p<=15)return 3;if(p<=19)return 4;if(p<=23)return 5;if(p<=24)return 6;if(p<=25)return 7;if(p<=27)return 8;return 9;}
    if(p<=9)return 1;if(p<=11)return 2;if(p<=13)return 3;if(p<=15)return 4;if(p<=19)return 5;if(p<=23)return 6;return 7;
}
int sony_recovered_plus_word_thresholds(const sony_plus_word_config *g,sony_plus_word_channel c[2],sony_plus_word_shared *s,int32_t budget){
    if(validate(g,c,s))return -1;const uint32_t row=threshold_row(g->channels,g->profile);
    s->threshold_limit=(int16_t)sf_wl_threshold_limits[row];s->tail_unit=(int16_t)sf_wl_threshold_tail[row];
    for(uint32_t ch=0;ch<g->channels;++ch){
        for(uint32_t q=0;q<32;++q)c[ch].input.threshold_index[q]=(int32_t)sf_wl_threshold_init[row*32u+q];
        if(g->sample_rate==48000)for(uint32_t q=18;q<g->units&&q<31;++q)c[ch].input.threshold_index[q]=(int32_t)sf_wl_threshold_init[row*32u+q+1];
        if(g->profile<=15){
            for(int32_t q=s->tail_unit;q<(int32_t)g->units;++q)if(c[ch].word_length[q]==1){
                const int32_t a=c[ch].significant[q];
                c[ch].input.threshold_index[q]+=(a<=6?5:0)+(a<=9)+(a<=12)+(a<=15)+(a<=18);
            }
            for(uint32_t q=18;q<g->units;++q)if(c[ch].word_length[q]==1){
                const int32_t a=c[ch].significant[q];if(a<=12||(a<=15&&q>=22)||(a<=18&&q>=24))++c[ch].input.threshold_index[q];
            }
        }
        for(uint32_t q=0;q<g->units;++q)if(c[ch].input.threshold_index[q]>15)c[ch].input.threshold_index[q]=15;
        if(sony_recovered_plus_unit_costs(&c[ch].input,c[ch].word_length,0,g->units,s->matrix+ch,s->channel_cost+ch))return -1;
    }
    update_total(g,s);
    if(mul(budget,constant(0x456628))>s->total)for(uint32_t ch=0;ch<g->channels;++ch)if(c[ch].maximum_word_length<7)c[ch].maximum_word_length=7;
    return 0;
}
int sony_recovered_plus_word_search(const sony_plus_word_config *g,sony_plus_word_channel c[2],sony_plus_word_shared *s,int32_t budget){
    if(validate(g,c,s))return -1;
    const uint32_t *decrease=((g->channels==2&&g->profile<=13)||(g->channels==1&&g->profile<=11))?sf_wl_wl_positive_low:sf_wl_wl_positive_high;
    float delta=s->total<budget?1.0f:-1.0f,step=1.0f,best=7.0f;int32_t direction=s->total<budget?1:-1;
    const float target=g->special_mode?0.75f:0.95f;
    if(g->special_mode)for(uint32_t ch=0;ch<g->channels;++ch){
        float maximum=0;for(uint32_t q=0;q<g->units;++q){c[ch].demand[q]=stored(mul(c[ch].demand[q],c[ch].demand[q]));if(c[ch].demand[q]>maximum)maximum=c[ch].demand[q];}
        if(maximum>1.0f){volatile double factor=(double)constant(0x4565d4)/maximum;for(uint32_t q=0;q<g->units;++q)c[ch].demand[q]=stored(mul(factor,c[ch].demand[q]));}
    }
    for(uint32_t iteration=0;iteration<8;++iteration){
        for(uint32_t ch=0;ch<g->channels;++ch){
            int32_t previous[32],changed[32];float scores[32];
            for(uint32_t q=0;q<32;++q)previous[q]=c[ch].word_length[q];
            for(uint32_t q=0;q<g->units;++q){
                if(g->special_mode)scores[q]=stored(add(delta,c[ch].demand[q]));
                /* Captured legacy table labels are misleading: 41c538 is
                 * selected for positive delta, 41c438/4b8 for nonpositive. */
                else{const uint32_t *curve=delta>0?sf_wl_wl_negative:decrease;uint32_t idx=q;if(g->sample_rate==48000&&q>=18&&q<31)++idx;
                    scores[q]=stored(add(mul(delta,number(curve[idx])),c[ch].demand[q]));}
            }
            if(sony_recovered_plus_initial_round(scores,c[ch].word_length,c[ch].maximum_word_length,(int32_t)g->units))return -1;
            if(!g->special_mode&&((g->channels==2&&g->profile>=13)||(g->channels==1&&g->profile>=11))){
                int32_t lower=c[ch].boundary_word;if(lower>3)lower=3;if(lower>c[ch].maximum_word_length)lower=c[ch].maximum_word_length;
                for(uint32_t q=0;q<g->units&&q<8;++q)if(c[ch].word_length[q]<lower)c[ch].word_length[q]=lower;
            }
            for(uint32_t q=0;q<g->units;++q){if(!c[ch].history[q])c[ch].word_length[q]=0;changed[q]=previous[q]!=c[ch].word_length[q];}
            if(sony_recovered_plus_unit_costs(&c[ch].input,c[ch].word_length,changed,g->units,s->matrix+ch,s->channel_cost+ch))return -1;
        }
        update_total(g,s);
        if(s->total>budget){if(delta<best)best=delta;}
        else if(s->total>(int32_t)mul(budget,target))break;
        if((delta>=constant(0x4565fc)&&s->total<=budget)||delta<=constant(0x456668)||(s->total<=budget&&iteration>=5))break;
        const int32_t next=s->total<budget?1:-1;
        if(iteration<7){
            if(next!=direction)step=stored(mul(step,constant(0x4565a8)));
            if(add(mul(next,step),delta)>=best)step=stored(mul(step,constant(0x4565a8)));
            delta=stored(add(mul(next,step),delta));
        }
        direction=next;
    }
    return 0;
}
int sony_recovered_plus_threshold_trim(const sony_plus_word_config *g,sony_plus_word_channel c[2],sony_plus_word_shared *s,int32_t budget){
    if(validate(g,c,s)||s->threshold_limit<0||s->threshold_limit>15||s->tail_unit<0||s->tail_unit>32)return -1;
    if(s->total<=budget)return 0;
    for(int32_t pass=0;pass<s->threshold_limit;++pass){
        int32_t changed[2][32]={{0}};
        for(int32_t q=(int32_t)g->units-1;q>=s->tail_unit;--q)for(uint32_t ch=0;ch<g->channels;++ch){
            update_total(g,s);if(s->total<=budget)break;
            if(c[ch].word_length[q]>0&&c[ch].input.threshold_index[q]<s->threshold_limit){
                if(++c[ch].input.threshold_index[q]>15){c[ch].input.threshold_index[q]=15;changed[ch][q]=0;}
                else{changed[ch][q]=1;c[ch].changed[q]=1;}
                if(sony_recovered_plus_unit_costs(&c[ch].input,c[ch].word_length,changed[ch],g->units,s->matrix+ch,s->channel_cost+ch))return -1;
            }
            changed[ch][q]=0;
        }
        if(s->total<=budget)break;
    }
    return 0;
}
int sony_recovered_plus_spectrum_trim(const sony_plus_word_config *g,sony_plus_word_channel c[2],sony_plus_word_shared *s,float *spectra[2],int32_t budget){
    if(validate(g,c,s)||!spectra)return -1;for(uint32_t ch=0;ch<g->channels;++ch)if(!spectra[ch])return -1;
    if(s->total<=budget)return 0;
    for(int32_t q=(int32_t)g->units-1;q>=0;--q){
        update_total(g,s);if(s->total<=budget)break;
        for(uint32_t ch=0;ch<g->channels;++ch)if(c[ch].word_length[q]>0){
            c[ch].input.threshold_index[q]=15;
            if(g->channels==2&&s->stereo_flags[q]==1){for(uint32_t i=0;i<sf_wl_qu_count[q];++i)spectra[0][sf_wl_qu_start[q]+i]=spectra[1][sf_wl_qu_start[q]+i]=0.0f;}
            else for(uint32_t i=0;i<sf_wl_qu_count[q];++i)spectra[ch][sf_wl_qu_start[q]+i]=0.0f;
            int32_t saved;if(sony_recovered_plus_zero_unit(c[ch].input.bank,(uint32_t)q,(uint32_t)c[ch].word_length[q],s->matrix+ch,&saved))return -1;
            s->channel_cost[ch]=word(s->channel_cost[ch]-saved);c[ch].changed[q]=1;
        }
    }
    return 0;
}
