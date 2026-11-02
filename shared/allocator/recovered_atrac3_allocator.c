#include "recovered_atrac3_allocator.h"
#include "../exponents/recovered_atrac3_exponents.h"
#include "../allocation/recovered_atrac3_allocation_helpers.h"
#include "../../atrac3/quant_cost.h"
#include "allocator_tables.inc"
static int32_t shifted(int32_t value,uint32_t shift)
{
    const uint32_t bits=(uint32_t)value;
    return (int32_t)((bits>>shift)|((bits&0x80000000u)?(~0u<<(32-shift)):0));
}
static int32_t exact_cost(const sony_atrac3_tonal_analysis_state *s,uint32_t q)
{
    return sony_recovered_atrac3_quant_cost(s->word_length[q],s->scale_factor[q],
        allocation_sizes[q],s->spectrum+allocation_starts[q]);
}
int32_t sony_recovered_atrac3_allocate(const sony_atrac3_allocation_config *c,
                                      sony_atrac3_tonal_analysis_state *s)
{
    if(!c||!s||!c->maximum_quantization_units||c->maximum_quantization_units>32||
       c->bit_budget<40||c->bit_budget>16384)return -1;
    for(uint32_t b=0;b<4;++b)if(c->gain_count[b]>7)return -1;
    for(uint32_t i=0;i<64;++i)if(s->components[i].scale_factor>63)return -1;
    uint32_t exponents[256],raw_sums[32];
    int32_t metric[32];
    sony_recovered_atrac3_exponents(s->spectrum,exponents);
    uint32_t count=c->maximum_quantization_units,significant=0,global_max=0;
    int32_t total_sum=0;
    for(uint32_t q=0;q<32;++q){
        uint32_t sum=0,maximum=0;
        for(uint32_t i=allocation_starts[q]/4;i<allocation_starts[q+1]/4;++i){
            const uint32_t v=exponents[i];sum+=v;if(maximum<v)maximum=v;
            if(v>7)++significant;
        }
        raw_sums[q]=sum;
        if(global_max<maximum){
            global_max=maximum;
            if(!c->extended_header&&
               (((!(allocation_starts[q]&128)||q>=22)&&
                  significant*8<(uint32_t)c->bit_budget)||
                  significant*16<(uint32_t)c->bit_budget)){
                const uint32_t desired=q<=26?28:(q+2<32?q+2:32);
                if(count<desired)count=desired;
            }
        }else if(sum<allocation_floors[q]||maximum<3){sum=0;maximum=0;}
        s->scale_factor[q]=maximum;metric[q]=(int32_t)significant;
        total_sum+=(int32_t)sum;s->word_length[q]=(uint32_t)total_sum;
    }
    s->quantization_units=count;
    const uint32_t precision=(uint32_t)c->bit_budget*2<(uint32_t)metric[count-1]*12?11:10;
    uint32_t bands;
    const int32_t gain_bits=sony_recovered_atrac3_gain_header_bits(count,c->gain_count,&bands);
    if(gain_bits<0)return -1;
    s->subbands=bands;
    int32_t remaining=c->bit_budget-(int32_t)(3*count)-gain_bits;
    if(metric[count-1]*32<remaining){
        const int32_t used=sony_recovered_atrac3_analyze_tonal_greedy(remaining,
            (uint32_t*)metric,exponents,s);
        if(used<0)return -2;
        remaining-=used;
    }else{
        for(uint32_t q=0;q<count;++q){
            int32_t value=(int32_t)s->scale_factor[q];
            uint32_t wl=0;
            if(value>=3){value=value*256-total_sum;
                wl=(uint32_t)sony_recovered_atrac3_clamp_word_length(value,precision,1);}
            s->word_length[q]=wl;metric[q]=value;
        }
        s->tonal_group_count=0;
        if(!c->extended_header){
            const int32_t used=sony_recovered_atrac3_analyze_tonal_alternate(
                remaining,(const uint32_t*)metric,s);
            if(used<0)return -2;
            remaining-=used;
        }
        for(uint32_t q=0;q<count;++q){
            const int32_t old=(int32_t)s->scale_factor[q];
            if(old<0){
                uint32_t maximum=0,sum=0;
                for(uint32_t i=allocation_starts[q]/4;i<allocation_starts[q+1]/4;++i){
                    const uint32_t v=sony_recovered_atrac3_exponent4(s->spectrum+4*i);
                    exponents[i]=v;sum+=v;if(maximum<v)maximum=v;
                }
                s->scale_factor[q]=maximum;
                if(!s->word_length[q])metric[q]+=256;
                else{
                    uint32_t index=(s->word_length[q]-1)>>1;if(index>4)index=4;
                    const int32_t base=(int32_t)allocation_adjacency[index]*((int32_t)maximum+old);
                    int32_t adjustment=base;
                    if(c->bit_budget<1100)adjustment=base*2;
                    else if(allocation_sizes[q]<=16&&metric[q]>6144){
                        adjustment=metric[q]-1024;
                        if(adjustment>base*2)adjustment=base*2;
                    }
                    if(q>1)metric[q-1]-=shifted(adjustment,3);
                    if(q<31)metric[q+1]-=shifted(adjustment,2);
                    metric[q]-=adjustment;
                }
                uint32_t wl=0;
                if(maximum>7&&sum>allocation_floors[q]&&
                   (metric[q]>=1024||c->bit_budget>1800))
                    wl=(uint32_t)sony_recovered_atrac3_clamp_word_length(metric[q],precision,1);
                s->word_length[q]=wl;
            }else if(precision==10){
                if((int32_t)allocation_sizes[q]*old>(int32_t)(raw_sums[q]*8))metric[q]+=256;
            }else{
                uint32_t sf=s->scale_factor[q];
                if(allocation_sizes[q]*sf<raw_sums[q]*5&&sf<62)++sf;
                if(sf<62)++sf;s->scale_factor[q]=sf;
            }
        }
    }
    int32_t estimated=0,active_samples=0,maximum_wl_samples=0;
    for(uint32_t q=0;q<count;++q){
        if(!s->word_length[q])continue;
        if((q>=2&&metric[q-1]-3840>metric[q])||
           (q+1<count&&metric[q+1]-5120>metric[q])){s->word_length[q]=0;continue;}
        active_samples+=(int32_t)allocation_sizes[q];
        if(s->word_length[q]==7)maximum_wl_samples+=(int32_t)allocation_sizes[q];
        const int32_t cost=sony_recovered_atrac3_heuristic_cost(q,exponents,
            s->word_length[q],s->scale_factor[q]);
        if(cost<0)return -2;estimated+=cost;
    }
    while(count>1&&!s->word_length[count-1]){remaining+=3;--count;}
    s->quantization_units=count;
    uint32_t flags[4][4]={{0}},kept_bands;
    for(uint32_t g=0;g<s->tonal_group_count&&g<2;++g)
        for(uint32_t b=0;b<4;++b)flags[g][b]=s->group[g].band_present[b];
    const int32_t refund=sony_recovered_atrac3_trim_bands(s->subbands,c->extended_header,
        c->header_field,s->tonal_group_count,c->gain_count,flags,&kept_bands);
    if(refund<0)return -2;s->subbands=kept_bands;remaining+=refund;
    const int32_t nominal=remaining*10;
    int32_t delta=0;
    int adjust=nominal<=estimated;
    if(!adjust){active_samples-=maximum_wl_samples;adjust=active_samples!=0;}
    if(adjust){
        const int32_t target=remaining>=600?remaining*9:nominal;
        if(!active_samples)return -2;
        delta=((target-estimated)*1024)/(active_samples*10);
        if(target-estimated<0&&precision==10){
            delta+=shifted(delta,3);
            metric[0]-=delta;metric[1]-=shifted(delta*188,8);
            for(uint32_t q=2;q<8;++q)metric[q]-=shifted(delta*97,8);
            for(uint32_t q=8;q<18;++q)metric[q]-=shifted(delta*74,8);
            for(uint32_t q=0;q<18;++q)
                if(metric[q]+delta<0&&s->scale_factor[q]<63)++s->scale_factor[q];
            for(uint32_t q=18;q<count;++q)if(metric[q]+delta<1024){
                int32_t sf=(int32_t)s->scale_factor[q]-shifted(metric[q]+delta-1024,10);
                if(sf>63)sf=63;s->scale_factor[q]=(uint32_t)sf;
            }
        }
    }
    int32_t descending_budget=nominal;
    for(uint32_t q=count;q-->0;){
        if(!s->word_length[q])continue;
        s->word_length[q]=(uint32_t)sony_recovered_atrac3_clamp_word_length(metric[q]+delta,precision,1);
        const int32_t cost=sony_recovered_atrac3_heuristic_cost(q,exponents,
            s->word_length[q],s->scale_factor[q]);
        if(cost<0)return -2;descending_budget-=cost;metric[q]=descending_budget;
    }
    int32_t used=0;uint32_t priority[32];
    for(uint32_t q=0;q<32;++q)priority[q]=raw_sums[q];
    for(uint32_t q=0;q<count;++q){
        priority[q]=UINT32_MAX;
        if(!s->word_length[q])continue;
        if((remaining-used)*2-16<(int32_t)allocation_sizes[q]){s->word_length[q]=0;continue;}
        const int32_t gap=metric[q]-used*10;
        const uint32_t wl=s->word_length[q];
        if(wl<7&&gap>4000)s->word_length[q]=wl+1;
        else if(q&&gap<300){
            if(wl<2){if(s->scale_factor[q]!=63)++s->scale_factor[q];}
            else s->word_length[q]=wl-1;
        }
        const int32_t cost=exact_cost(s,q);if(cost<0)return -2;
        metric[q]=cost;
        if(cost*2==(int32_t)(allocation_clc[s->word_length[q]]*allocation_sizes[q])+12)
            s->word_length[q]=0;
        else {used+=cost;priority[q]=s->scale_factor[q];}
    }
    while(count>1&&!s->word_length[count-1]){remaining+=3;--count;}
    s->quantization_units=count;
    int32_t free_bits=remaining-used;
    if(((uint32_t)c->refinement_trigger+c->gain_count[0])&&!c->extended_header)
        for(uint32_t q=0;q<5;++q)if(priority[q]<65)priority[q]=64;
    int32_t spent=0;
    for(;;){
        int32_t highest=-1;uint32_t selected=0;
        for(uint32_t q=0;q<count;++q)if((int32_t)priority[q]>highest){
            highest=(int32_t)priority[q];selected=q;}
        if(highest<0)return c->bit_budget-remaining+spent;
        const uint32_t q=selected;
        int32_t cost=metric[q];priority[q]=UINT32_MAX;
        if(remaining<spent+cost){
            const uint32_t size=allocation_sizes[q],clc=allocation_clc[s->word_length[q]];
            uint32_t last=count-1;
            while(q<last){
                if(s->word_length[last]&&priority[last]==UINT32_MAX)break;
                s->word_length[last]=0;remaining+=3;priority[last]=UINT32_MAX;--last;
            }
            count=last+1;s->quantization_units=count;
            while(remaining<spent+cost){
                ++s->scale_factor[q];
                if(s->scale_factor[q]>=64||cost<=(int32_t)(clc*(size>>1))+6)break;
                cost=exact_cost(s,q);if(cost<0)return -2;
            }
        }else if(s->word_length[q]<7&&
            (int32_t)(allocation_clc[s->word_length[q]+1]*(allocation_sizes[q]>>1))<remaining-spent-7){
            uint32_t next=s->word_length[q]+1;
            if((int32_t)(allocation_sizes[q]*4)<free_bits&&next<7)++next;
            const uint32_t old=s->word_length[q];s->word_length[q]=next;
            const int32_t trial=exact_cost(s,q);s->word_length[q]=old;
            if(trial<0)return -2;
            if(trial-cost<=(free_bits>0?free_bits:0)){
                s->word_length[q]=next;free_bits+=cost-trial;cost=trial;
            }
        }
        spent+=cost;
        if(remaining<spent){spent-=cost;free_bits+=cost;s->word_length[q]=0;}
    }
}
