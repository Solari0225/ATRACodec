#include "recovered_plus_allocator.h"
#include "../plus_mode_cost/independent_plus_flag_cost.h"
#include "../plus_quantization/recovered_plus_quantization.h"
#include "energy_tables.inc"
static int16_t word(int32_t x){return (int16_t)(uint16_t)x;}
static void normalized(sony_plus_allocator_state *s,uint32_t n){for(uint32_t ch=0;ch<n;++ch)for(uint32_t i=0;i<2048;++i)s->channel[ch].input.normalized[i]=s->stereo.spectrum[ch][i];}
static void sum_total(sony_plus_allocator_state *s,uint32_t n){int32_t total=s->costs[9];for(uint32_t ch=0;ch<n;++ch)total+=s->costs[12+ch];s->costs[11]=word(total);}
int sony_recovered_plus_allocator_initialize(const sony_plus_allocator_config *g,sony_plus_allocator_state *s,int32_t budget){
    if(!g||!s||g->initial.channels<1||g->initial.channels>2||g->full_table>1)return -1;
    const uint32_t n=g->initial.channels;sony_plus_initial_channel initial[2]={s->channel[0].analysis,s->channel[1].analysis};
    if(sony_recovered_plus_initial_words(&g->initial,initial,&s->stereo,&s->limits))return -1;
    s->budget=budget;s->scale_modes_enabled=(int32_t)s->limits.format_flag;normalized(s,n);
    sony_plus_sf_choice_channel sf[2]={s->channel[0].sf,s->channel[1].sf};
    for(uint32_t ch=0;ch<n;++ch){s->channel[ch].analysis=initial[ch];s->channel[ch].input.bank=initial[ch].noise_flag;for(uint32_t q=0;q<32;++q){sf[ch].word_length[q]=(int32_t)initial[ch].word_length[q];s->channel[ch].words.raw[q]=initial[ch].word_length[q];sf[ch].coarse[q]=s->stereo.coarse[ch][q];}}
    sony_plus_sf_choice_config cfg={n,s->limits.active_units,g->scale_groups,g->full_table};
    s->costs[0]=6;s->costs[1]=word((3*(int32_t)s->limits.limit_units+2)*(int32_t)n);int32_t bits;
    if(sony_recovered_plus_choose_scale_factors(&cfg,sf,s->scale_modes_enabled,&bits))return -1;s->costs[2]=word(bits);
    if(sony_recovered_plus_codebook_class_cost(&cfg,sf,&bits))return -1;s->costs[3]=word(bits);
    for(uint32_t ch=0;ch<n;++ch)s->channel[ch].sf=sf[ch];
    plus_given_gain_channel gains[2]={s->channel[0].gain,s->channel[1].gain};plus_given_gain_location_parameters loc[2]={s->channel[0].gain_location,s->channel[1].gain_location};
    bits=plus_gain_choose_modes(gains,loc,(int32_t)n,1);if(bits<0)return -1;s->costs[6]=word(bits);
    s->costs[5]=0;
    for(uint32_t ch=0;ch<n;++ch){s->channel[ch].gain=gains[ch];s->channel[ch].gain_location=loc[ch];sony_plus_allocator_flags *f=&s->channel[ch].window;plus_given_flag_vector v={f->present,f->explicit_values,f->values};bits=plus_mode_choose_flags(&v,(int32_t)s->limits.limit_subbands);if(bits<0)return -1;f->present=v.present;f->explicit_values=v.explicit_values;s->costs[5]=word(s->costs[5]+bits);}
    s->costs[4]=0;
    if(s->limits.active_units>2)s->costs[4]=word(4*(int32_t)n*(energy_band_region[s->limits.active_subbands-1]+1));
    if(n==2)for(uint32_t j=0;j<2;++j){uint32_t flags[16];for(uint32_t b=0;b<16;++b)flags[b]=j?s->stereo_second[b]:(uint32_t)s->stereo.band_output[b];plus_given_flag_vector v={s->stereo_modes[j][0],s->stereo_modes[j][1],flags};bits=plus_mode_choose_flags(&v,(int32_t)s->limits.active_subbands);if(bits<0)return -1;s->stereo_modes[j][0]=v.present;s->stereo_modes[j][1]=v.explicit_values;s->costs[4]=word(s->costs[4]+bits);}
    bits=plus_tone_choose_modes_v2(&s->tones,1);if(bits<0)return -1;s->costs[7]=word(bits);s->costs[8]=g->noise_present?9:1;
    int32_t total=0;for(uint32_t i=0;i<9;++i)total+=s->costs[i];s->costs[9]=word(total);return 0;
}
static void to_words(const sony_plus_allocator_state *s,uint32_t n,sony_plus_word_channel c[2],sony_plus_word_shared *shared){
    for(uint32_t ch=0;ch<2;++ch){const sony_plus_allocator_channel *a=s->channel+ch;c[ch].input=a->input;c[ch].maximum_word_length=(int16_t)a->analysis.maximum_word_length;c[ch].boundary_word=(int16_t)a->analysis.activity;for(uint32_t q=0;q<32;++q){c[ch].word_length[q]=(int32_t)a->analysis.word_length[q];c[ch].significant[q]=a->analysis.significant[q];c[ch].history[q]=a->analysis.history[q];c[ch].demand[q]=a->analysis.demand[q];c[ch].changed[q]=0;}shared->matrix[ch]=s->matrix[ch];shared->channel_cost[ch]=s->costs[12+ch];}
    shared->threshold_limit=s->threshold_limit;shared->tail_unit=s->tail_unit;shared->overhead=s->costs[9];shared->auxiliary_total=s->costs[10];shared->total=s->costs[11];for(uint32_t q=0;q<32;++q)shared->stereo_flags[q]=(int16_t)s->limits.stereo_flags[q];(void)n;
}
static void from_words(sony_plus_allocator_state *s,uint32_t n,const sony_plus_word_channel c[2],const sony_plus_word_shared *shared){
    for(uint32_t ch=0;ch<n;++ch){sony_plus_allocator_channel *a=s->channel+ch;a->input=c[ch].input;a->analysis.maximum_word_length=c[ch].maximum_word_length;for(uint32_t q=0;q<32;++q){a->analysis.word_length[q]=(uint32_t)c[ch].word_length[q];a->analysis.demand[q]=c[ch].demand[q];a->sf.word_length[q]=c[ch].word_length[q];}s->matrix[ch]=shared->matrix[ch];s->costs[12+ch]=shared->channel_cost[ch];}
    s->threshold_limit=shared->threshold_limit;s->tail_unit=shared->tail_unit;s->costs[9]=shared->overhead;s->costs[10]=shared->auxiliary_total;s->costs[11]=shared->total;
}
static int quantize(sony_plus_allocator_state *s,uint32_t n,uint32_t units,uint32_t simd,const sony_plus_word_channel *changed){
    for(uint32_t ch=0;ch<n;++ch)for(uint32_t q=0;q<units;++q)if(s->channel[ch].analysis.word_length[q]>0&&(!changed||changed[ch].changed[q])){sony_plus_allocator_channel *a=s->channel+ch;if(sony_recovered_plus_quantize(s->stereo.spectrum[ch]+energy_qu_start[q],a->quantized+energy_qu_start[q],a->analysis.word_length[q],(uint32_t)a->input.threshold_index[q],a->input.threshold_factor[q],energy_qu_count[q],simd))return -1;}return 0;
}
int32_t sony_recovered_plus_allocate(const sony_plus_allocator_config *g,sony_plus_allocator_state *s,int32_t budget){
    if(!g||!s||g->initial.channels<1||g->initial.channels>2)return -1;const uint32_t n=g->initial.channels,units=g->initial.maximum_units;
    s->word_modes_enabled=!g->initial.special_mode&&g->initial.profile<27;
    for(uint32_t ch=0;ch<n;++ch)s->channel[ch].words.mode=0;
    if(sony_recovered_plus_allocator_initialize(g,s,budget))return -1;
    sony_plus_word_channel wc[2];sony_plus_word_shared ws;to_words(s,n,wc,&ws);sony_plus_word_config wg={n,units,g->initial.sample_rate,g->initial.profile,g->initial.special_mode};
    if(sony_recovered_plus_word_thresholds(&wg,wc,&ws,budget)||sony_recovered_plus_word_search(&wg,wc,&ws,budget))return -1;from_words(s,n,wc,&ws);sum_total(s,n);
    uint32_t wl[2][32];for(uint32_t ch=0;ch<2;++ch)for(uint32_t q=0;q<32;++q)wl[ch][q]=s->channel[ch].analysis.word_length[q];plus_fast_wl_parameters modes[2]={s->channel[0].modes,s->channel[1].modes};
    int32_t bits=plus_fast_wl_choose(wl,(int32_t)s->limits.limit_units,(int32_t)n,1,0,s->word_modes_enabled,modes);if(bits<0)return -1;const int32_t delta=bits-s->costs[1];s->costs[1]=word(bits);s->costs[9]=word(s->costs[9]+delta);s->costs[11]=word(s->costs[11]+delta);s->channel[0].modes=modes[0];s->channel[1].modes=modes[1];
    ws.overhead=s->costs[9];ws.total=s->costs[11];float *spectra[2]={s->stereo.spectrum[0],s->stereo.spectrum[1]};
    if(sony_recovered_plus_threshold_trim(&wg,wc,&ws,budget)||sony_recovered_plus_spectrum_trim(&wg,wc,&ws,spectra,budget))return -1;from_words(s,n,wc,&ws);normalized(s,n);
    for(uint32_t ch=0;ch<n;++ch)for(uint32_t q=0;q<32;++q)s->channel[ch].selected_book[q]=s->matrix[ch].book[q];
    for(uint32_t q=0;q<units;++q){if(n==2){if(s->limits.swap_flags[q])s->channel[1].selected_book[q]=1;if(!s->channel[0].analysis.word_length[q]&&!s->channel[1].analysis.word_length[q])s->channel[0].selected_book[q]=s->channel[1].selected_book[q]=0;}else if(!s->channel[0].analysis.word_length[q])s->channel[0].selected_book[q]=0;}
    sum_total(s,n);sony_plus_greedy_channel gc[2];sony_plus_greedy_shared gs;sony_plus_greedy_config gg={n,units,s->limits.limit_units,g->initial.special_mode};gs.reserve=s->reserve;gs.enabled=s->word_modes_enabled;gs.word_bits=s->costs[1];gs.overhead=s->costs[9];gs.total=s->costs[11];
    for(uint32_t ch=0;ch<2;++ch){gc[ch].input=s->channel[ch].input;gc[ch].maximum_word_length=s->channel[ch].analysis.maximum_word_length;gc[ch].modes=s->channel[ch].modes;gs.matrix[ch]=s->matrix[ch];gs.channel_cost[ch]=s->costs[12+ch];for(uint32_t q=0;q<32;++q){gc[ch].word_length[q]=(int32_t)s->channel[ch].analysis.word_length[q];gc[ch].selected_book[q]=s->channel[ch].selected_book[q];gc[ch].priority.coarse[q]=s->channel[ch].sf.coarse[q];gc[ch].priority.peakiness[q]=s->channel[ch].analysis.peakiness[q];}}
    if(sony_recovered_plus_greedy_words(&gg,gc,&gs,budget))return -1;
    for(uint32_t ch=0;ch<n;++ch){s->channel[ch].modes=gc[ch].modes;s->matrix[ch]=gs.matrix[ch];s->costs[12+ch]=gs.channel_cost[ch];for(uint32_t q=0;q<32;++q){s->channel[ch].analysis.word_length[q]=(uint32_t)gc[ch].word_length[q];s->channel[ch].sf.word_length[q]=gc[ch].word_length[q];s->channel[ch].selected_book[q]=gc[ch].selected_book[q];}}
    s->costs[1]=gs.word_bits;s->costs[9]=gs.overhead;s->costs[11]=gs.total;
    sony_plus_noise_region_config ng={n,s->limits.active_units,s->limits.active_subbands,g->initial.profile,{0}};sony_plus_noise_region_channel nc[2]={{0}};
    const int32_t noise_enabled=!g->initial.special_mode&&g->initial.profile<=(n==2?23u:19u);
    for(uint32_t b=0;b<16;++b)ng.stereo_flags[b]=s->stereo_second[b];
    /* The noise helper's historical scale_factor field maps codec+1b578:
     * it is the selected short codebook, not coarse SF at codec+1b678. */
    for(uint32_t ch=0;ch<n;++ch){nc[ch].active=noise_enabled;for(uint32_t q=0;q<32;++q){nc[ch].metric[q]=s->channel[ch].analysis.peakiness[q];nc[ch].integer_bias[q]=s->channel[ch].input.threshold_index[q];nc[ch].word_length[q]=(int32_t)s->channel[ch].analysis.word_length[q];nc[ch].scale_factor[q]=s->channel[ch].selected_book[q];}}
    if(sony_recovered_plus_noise_regions(&ng,nc))return -1;
    for(uint32_t ch=0;ch<n;++ch){sony_plus_allocator_channel *a=s->channel+ch;for(uint32_t j=0;j<5;++j)a->noise_region[j]=nc[ch].region[j];if(a->modes.mode){const plus_fast_wl_method *m=a->modes.method+a->modes.mode-1;a->words.mode=(uint32_t)a->modes.mode;a->words.weight_id=0;a->words.huffman_id=(uint32_t)m->huffman_id;a->words.range_mode=(uint32_t)m->range_mode;a->words.packed_count=(uint32_t)m->encoded_units;a->words.fill_extra=(uint32_t)a->method_fill_extra[a->modes.mode-1];}}
    if(quantize(s,n,units,g->simd,0))return -1;
    uint32_t banks[2]={s->channel[0].input.bank,s->channel[1].input.bank};int32_t words[2][32],minimum;
    for(uint32_t ch=0;ch<2;++ch)for(uint32_t q=0;q<32;++q)words[ch][q]=(int32_t)s->channel[ch].analysis.word_length[q];
    if(sony_recovered_plus_minimum_spectrum_cost(n,units,banks,words,&minimum))return -1;
    sony_plus_refine_config rg={{n,s->limits.active_units,g->scale_groups,g->full_table},{n,{0}},units,s->limits.active_subbands,g->initial.profile,g->simd,{0}};sony_plus_refine_channel rc[2];sony_plus_refine_shared rs={s->scale_modes_enabled,s->costs[2],s->costs[9],s->costs[11]};
    for(uint32_t b=0;b<16;++b)rg.noise.stereo_band[b]=s->stereo_second[b];for(uint32_t q=0;q<32;++q)rg.stereo_units[q]=s->limits.stereo_flags[q];
    for(uint32_t ch=0;ch<2;++ch){const sony_plus_allocator_channel *a=s->channel+ch;rc[ch].sf=a->sf;rc[ch].noise_enabled=(uint32_t)noise_enabled;for(uint32_t i=0;i<2048;++i){rc[ch].spectrum[i]=s->stereo.spectrum[ch][i];rc[ch].quantized[i]=a->quantized[i];}for(uint32_t q=0;q<32;++q)rc[ch].peakiness[q]=a->analysis.peakiness[q];for(uint32_t b=0;b<16;++b){const plus_given_gain_band *p=a->gain.bands+b;rc[ch].noise.current[b].count=p->points;rc[ch].noise.current[b].window_flag=a->window.values[b];for(uint32_t j=0;j<7;++j){rc[ch].noise.current[b].location[j]=p->locations[j];rc[ch].noise.current[b].level[j]=p->levels[j];}rc[ch].noise.previous[b]=a->previous_gain[b];}for(uint32_t j=0;j<5;++j)rc[ch].noise.region[j]=a->noise_region[j];}
    if(sony_recovered_plus_refine_scales(&rg,rc,&rs,budget-s->costs[9]-minimum))return -1;
    for(uint32_t ch=0;ch<n;++ch){s->channel[ch].sf=rc[ch].sf;for(uint32_t q=0;q<32;++q)s->stereo.coarse[ch][q]=rc[ch].sf.coarse[q];}s->costs[2]=rs.sf_bits;s->costs[9]=rs.overhead;s->costs[11]=rs.total;
    if(s->costs[11]>budget){
        to_words(s,n,wc,&ws);for(uint32_t ch=0;ch<n;++ch)for(uint32_t q=0;q<32;++q)ws.matrix[ch].book[q]=s->channel[ch].selected_book[q];
        if(sony_recovered_plus_threshold_trim(&wg,wc,&ws,budget)||sony_recovered_plus_spectrum_trim(&wg,wc,&ws,spectra,budget))return -1;from_words(s,n,wc,&ws);normalized(s,n);sum_total(s,n);for(uint32_t ch=0;ch<n;++ch)for(uint32_t q=0;q<32;++q)s->channel[ch].selected_book[q]=s->matrix[ch].book[q];if(quantize(s,n,units,g->simd,wc))return -1;
    }
    for(uint32_t ch=0;ch<n;++ch)for(uint32_t q=0;q<32;++q)s->channel[ch].words.raw[q]=s->channel[ch].words.values[q]=s->channel[ch].analysis.word_length[q];
    s->actual_bits=s->costs[11];return s->actual_bits;
}
