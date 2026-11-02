#include "recovered_plus_greedy.h"
static int16_t word(int32_t x){return (int16_t)(uint16_t)x;}
int sony_recovered_plus_greedy_words(const sony_plus_greedy_config *g,sony_plus_greedy_channel c[2],sony_plus_greedy_shared *s,int32_t budget){
    if(!g||!c||!s||g->channels<1||g->channels>2||g->units>32||g->word_units>32)return -1;
    sony_plus_priority_channel pr[2]={c[0].priority,c[1].priority};int32_t scores[64],order[64];
    for(uint32_t ch=0;ch<g->channels;++ch)if(c[ch].maximum_word_length<1||c[ch].maximum_word_length>7)return -1;
    if(sony_recovered_plus_priorities(g->channels,g->units,g->special_mode,pr,scores,order))return -1;
    if(s->total>budget-s->reserve)return 0;
    int32_t eligible[2][32]={{0}},attempts=0;
    for(uint32_t ch=0;ch<g->channels;++ch)for(uint32_t q=0;q<g->units;++q)eligible[ch][q]=1;
    for(uint32_t pass=0;pass<7;++pass)for(uint32_t rank=0;rank<g->channels*g->units;++rank){
        if(attempts==20)break;
        const uint32_t ch=(uint32_t)order[rank]/g->units,q=(uint32_t)order[rank]%g->units;
        if(s->total>budget-s->reserve||!eligible[ch][q])continue;
        const int32_t previous=c[ch].word_length[q];
        if(previous<=0||previous>=c[ch].maximum_word_length||c[ch].input.threshold_index[q]>=15)continue;
        c[ch].word_length[q]=previous+1;++attempts;
        sony_plus_cost_matrix trial={0};int32_t best,delta;
        if(sony_recovered_plus_trial_cost(q,(uint32_t)c[ch].word_length[q],c[ch].selected_book[q],&c[ch].input,s->matrix+ch,&trial,&best,&delta))return -1;
        if(s->total+delta>budget){c[ch].word_length[q]=previous;eligible[ch][q]=0;continue;}
        plus_fast_wl_parameters before[2]={c[0].modes,c[1].modes},modes[2]={c[0].modes,c[1].modes};uint32_t wl[2][32];
        for(uint32_t a=0;a<2;++a)for(uint32_t i=0;i<32;++i)wl[a][i]=(uint32_t)c[a].word_length[i];
        const int32_t cost=plus_fast_wl_choose(wl,(int32_t)g->word_units,(int32_t)g->channels,0,(int32_t)ch,s->enabled,modes);if(cost<0)return -1;
        delta+=cost-s->word_bits;
        if(s->total+delta<=budget){
            for(uint32_t i=0;i<4;++i)s->matrix[ch].costs[q][i]=trial.costs[q][i];
            s->overhead=word(s->overhead+cost-s->word_bits);
            s->channel_cost[ch]=word(s->channel_cost[ch]+delta+s->word_bits-cost);
            s->word_bits=word(cost);s->total=word(s->total+delta);c[ch].selected_book[q]=best;
            c[0].modes=modes[0];c[1].modes=modes[1];
        }else{
            c[ch].word_length[q]=previous;eligible[ch][q]=0;c[0].modes=before[0];c[1].modes=before[1];
        }
    }
    return 0;
}
