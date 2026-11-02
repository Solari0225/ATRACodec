#include "recovered_atrac3_tonal_analysis.h"
#include "../tonal/recovered_atrac3_tonal.h"
#include "../exponents/recovered_atrac3_exponents.h"
#include "tonal_analysis_tables.inc"
static int above_threshold(float value)
{
    union {float value;uint32_t bits;} data;
    data.value=value;return (data.bits<<1)>=(GREEDY_THRESHOLD_WORD<<1);
}
/* Original464530. Ties preserve the earlier incumbent. Removal leaves the
 * duplicated trailing ID untouched, matching the original pointer shift. */
static uint32_t replace_weaker(sony_atrac3_tonal_analysis_state *state,
    uint32_t group_index,uint32_t quarter,uint32_t incoming_estimate)
{
    sony_atrac3_tonal_analysis_group *group=state->group+group_index;
    sony_atrac3_tonal_slots *window=group->window+quarter;
    int32_t chosen=-1;
    uint32_t minimum=incoming_estimate;
    for(uint32_t i=0;i<7;++i){
        const uint32_t id=window->component_ids[i];
        const uint32_t sf=state->components[id-1].scale_factor;
        if(minimum>sf){minimum=sf;chosen=(int32_t)i;}
    }
    if(chosen<0)return 0;
    const uint32_t id=window->component_ids[chosen];
    if(sony_recovered_atrac3_restore_tonal(state->spectrum,
       state->components+id-1,group->word_length,
       group->coefficients_minus_one+1))return 0;
    for(uint32_t i=(uint32_t)chosen;i<6;++i)
        window->component_ids[i]=window->component_ids[i+1];
    --window->count;return id;
}
int32_t sony_recovered_atrac3_analyze_tonal_greedy(int32_t available_bits,
    uint32_t metrics[32],uint32_t estimates[256],
    sony_atrac3_tonal_analysis_state *state)
{
    if(!state||!metrics||!estimates||available_bits<0||
       !state->quantization_units||state->quantization_units>32||
       !state->subbands||state->subbands>4)return -1;
    for(uint32_t i=0;i<64;++i)if(state->components[i].scale_factor>63)return -1;
    int32_t cost=22;
    const uint32_t end=tonal_unit_starts[state->quantization_units];
    const uint32_t minimum_bands=end>>8;
    if(state->subbands<minimum_bands){
        cost+=3*(int32_t)(minimum_bands-state->subbands);
        state->subbands=minimum_bands;
    }
    for(uint32_t g=0;g<2;++g){
        sony_atrac3_tonal_analysis_group *group=state->group+g;
        for(uint32_t b=0;b<4;++b)group->band_present[b]=0;
        group->word_length=g?7:5;group->coefficients_minus_one=3;
        for(uint32_t q=0;q<16;++q){
            group->window[q].count=0;
            for(uint32_t i=0;i<7;++i)group->window[q].component_ids[i]=0;
        }
    }
    state->tonal_mode=1;state->tonal_group_count=2;
    uint32_t used=0;
    int stop=0;
    for(;;){
        const uint32_t previous=used;
        for(uint32_t i=0;i<end;){
            if(!above_threshold(state->spectrum[i])){++i;continue;}
            const uint32_t pending_id=used+1;
            if(pending_id>64)return -2;
            sony_atrac3_tonal_component *pending=state->components+used;
            pending->position=i<1020?i:1020;
            const uint32_t coarse=sony_recovered_atrac3_exponent4(
                state->spectrum+pending->position);
            const uint32_t quarter=i>>6;
            uint32_t group_index=coarse>30?1:0;
            uint32_t id=pending_id;
            ++used;
            if(state->group[group_index].window[quarter].count==7){
                group_index=1-group_index;
                if(state->group[group_index].window[quarter].count==7){
                    group_index=1-group_index;
                    --used;
                    id=replace_weaker(state,group_index,quarter,coarse);
                    if(!id){++i;continue;}
                }
            }
            sony_atrac3_tonal_analysis_group *group=state->group+group_index;
            if(!group->band_present[quarter>>2]){
                group->band_present[quarter>>2]=1;cost+=12;
            }
            sony_atrac3_tonal_slots *window=group->window+quarter;
            if(window->count>=7)return -2;
            window->component_ids[window->count++]=id;
            sony_atrac3_tonal_component *component=state->components+id-1;
            if(sony_recovered_atrac3_quantize_tonal(
                state->spectrum+component->position,component,
                group->word_length,4)<0)return -2;
            cost+=(int32_t)(28+8*group_index);
            if(cost>available_bits-200||used>=64){stop=1;break;}
            i+=4;
        }
        if(stop||previous==used)break;
    }
    for(uint32_t unit=0;unit<state->quantization_units;++unit){
        uint32_t maximum=0;
        for(uint32_t q=tonal_unit_starts[unit]/4;
            q<tonal_unit_starts[unit+1]/4;++q){
            const uint32_t value=sony_recovered_atrac3_exponent4(state->spectrum+4*q);
            estimates[q]=value;if(maximum<value)maximum=value;
        }
        state->scale_factor[unit]=maximum;
        state->word_length[unit]=maximum!=0;
        metrics[unit]=341*maximum;
    }
    return cost;
}
static int32_t shift_leading_symbols(sony_atrac3_tonal_component *c,uint32_t unit)
{
    uint32_t shift=1;
    while(shift<4&&!c->symbol[shift])++shift;
    const uint32_t position=c->position+shift;
    if(position>=tonal_unit_starts[unit+1])return -1;
    if(position>1020)return 0;
    if(shift==4)c->symbol[0]=c->position;
    else for(uint32_t i=0;i<4-shift;++i)c->symbol[i]=c->symbol[i+shift];
    for(uint32_t i=4-shift;i<4;++i)if(i)c->symbol[i]=0;
    c->position=position;return (int32_t)shift;
}
static uint32_t split_components(sony_atrac3_tonal_analysis_state *state,
                                 uint32_t count)
{
    state->group[1]=state->group[0];
    uint32_t prospective=count;
    for(uint32_t i=0;i<count;++i){
        const sony_atrac3_tonal_component *c=state->components+i;
        if(!c->symbol[3])continue;
        const uint32_t position=c->position+2,quarter=position>>6;
        if(quarter>15||!state->group[1].band_present[quarter>>2]||
           state->group[1].window[quarter].count>=7||prospective==64||
           position>1021)return 0;
        ++state->group[1].window[quarter].count;++prospective;
    }
    uint32_t total=count;
    for(uint32_t i=0;i<count;++i){
        sony_atrac3_tonal_component *c=state->components+i;
        if(!c->symbol[3])continue;
        sony_atrac3_tonal_component *next=state->components+total;
        next->position=c->position+2;next->scale_factor=c->scale_factor;
        next->symbol[0]=c->symbol[2];next->symbol[1]=c->symbol[3];
        c->symbol[2]=0;c->symbol[3]=0;next->symbol[2]=0;next->symbol[3]=0;
        const uint32_t quarter=next->position>>6;
        sony_atrac3_tonal_slots *window=state->group[0].window+quarter;
        window->component_ids[window->count++]=++total;
    }
    return total;
}
static int32_t trim_component_width(sony_atrac3_tonal_analysis_state *state,
                                     uint32_t count)
{
    uint32_t width=state->group[0].coefficients_minus_one;
    for(;;){
        if(!width||width>3)return -1;
        uint32_t needed=0;
        for(uint32_t index=count;index-->0;){
            sony_atrac3_tonal_component *c=state->components+index;
            if(c->symbol[width]){
                ++c->position;
                for(uint32_t j=0;j<width;++j)c->symbol[j]=c->symbol[j+1];
            }
            if(c->symbol[width-1]&&
               (c->symbol[0]||(c->position&63)==63))needed=1;
        }
        --width;state->group[0].coefficients_minus_one=width;
        if(needed)return (int32_t)(3-width);
    }
}
int32_t sony_recovered_atrac3_analyze_tonal_alternate(int32_t available_bits,
    const uint32_t metrics[32],sony_atrac3_tonal_analysis_state *state)
{
    if(!state||!metrics||available_bits<0||!state->quantization_units||
       state->quantization_units>32||!state->subbands||state->subbands>4||
       (available_bits<1100&&state->quantization_units<=8))return -1;
    for(uint32_t i=0;i<64;++i)if(state->components[i].scale_factor>63)return -1;
    sony_atrac3_tonal_analysis_group *group=state->group;
    for(uint32_t b=0;b<4;++b)group->band_present[b]=0;
    group->word_length=3;group->coefficients_minus_one=3;
    for(uint32_t q=0;q<16;++q){group->window[q].count=0;
        for(uint32_t i=0;i<7;++i)group->window[q].component_ids[i]=0;}
    uint32_t count=0,zero_last=0;
    int32_t component_bits=0,header_bits=8;
    for(uint32_t unit=available_bits<1100?8:0;
        unit<state->quantization_units;++unit){
        const uint32_t size=tonal_unit_starts[unit+1]-tonal_unit_starts[unit];
        if(count>64-(size>>4)||
           (int32_t)(24*count)+header_bits>available_bits-600)break;
        const uint32_t wl=state->word_length[unit];
        if(!wl)continue;
        if(wl>7||state->scale_factor[unit]>63)return -1;
        const uint32_t begin=tonal_unit_starts[unit];
        uint32_t band=begin>>8;if(size==32)band=1;
        /* Original CMP/JGE is signed: negative demand selects column0. */
        const uint32_t table_index=8*band+((int32_t)metrics[unit]<1280?0:wl);
        const uint32_t sf=state->scale_factor[unit],adjust=alternate_threshold_adjust[table_index];
        const uint32_t threshold=sf>adjust?sf-adjust:0;
        uint32_t scan=begin;
        if(count&&state->components[count-1].position+4>begin){
            const uint32_t overlap=state->components[count-1].position+4-begin;
            state->scale_factor[unit]=0u-sf;state->word_length[unit]=overlap;
            state->word_length[unit-1]+=overlap-4;
            scan=state->components[count-1].position+4;
        }
        uint32_t candidates[9];
        /* Valid caller state keeps scan below this QU end. A prior component
         * can consume the entire8-sample unit only for invalid custom state. */
        if(scan>=tonal_unit_starts[unit+1])continue;
        const int32_t found=sony_recovered_atrac3_collect_tonal_peaks(
            state->spectrum,threshold,scan,tonal_unit_starts[unit+1],
            candidates,(size+8)>>4);
        if(found<=0)continue;
        if((int32_t)state->scale_factor[unit]>=0){
            state->scale_factor[unit]=0u-state->scale_factor[unit];
            state->word_length[unit]=0;
        }
        for(uint32_t candidate=0;candidate<(uint32_t)found;++candidate){
            uint32_t position=candidates[candidate];if(position>1020)position=1020;
            uint32_t newly_used=4;
            if(count&&state->components[count-1].position+4==position){
                while(newly_used>1){
                    const float left=state->spectrum[position-1],right=state->spectrum[position+3];
                    const float a=left<0?-left:left,b=right<0?-right:right;
                    if(b>a)break;
                    --position;--newly_used;
                }
            }
            uint32_t quarter=position>>6;
            if(group->window[quarter].count>=7)continue;
            if(count>=64)return -2;
            sony_atrac3_tonal_component *c=state->components+count;
            ++count;c->position=position;
            const int32_t cost=sony_recovered_atrac3_quantize_tonal(
                state->spectrum+position,c,3,4);
            if(cost<0)return -2;
            if(!c->symbol[0]&&c->position<=1020){
                if(shift_leading_symbols(c,unit)<0){
                    if(sony_recovered_atrac3_restore_tonal(state->spectrum,c,3,4))return -2;
                    --count;break;
                }
                quarter=c->position>>6;
            }
            component_bits+=cost;state->word_length[unit]+=newly_used;
            if(!group->band_present[quarter>>2]){
                group->band_present[quarter>>2]=1;header_bits+=12;
            }
            sony_atrac3_tonal_slots *window=group->window+quarter;
            window->component_ids[window->count++]=count;
            if(!c->symbol[3])++zero_last;
        }
    }
    if(!count)return 0;
    if(count==1&&(state->components[0].position<128||available_bits<1100)){
        if(sony_recovered_atrac3_restore_tonal(state->spectrum,state->components,3,4))return -2;
        return 0;
    }
    header_bits+=(int32_t)state->subbands;state->tonal_group_count=1;
    int32_t fixed_bits=(int32_t)(24*count);
    int common_trim=zero_last==count;
    if(!common_trim){
        const int32_t samples=(int32_t)(2*count)-(int32_t)(3*zero_last);
        const int32_t overhead=(int32_t)(12*(count-zero_last));
        const int32_t alternative_vlc=samples+overhead+component_bits;
        const int32_t alternative_fixed=3*samples+overhead+fixed_bits;
        const int32_t current_minimum=component_bits<fixed_bits?component_bits:fixed_bits;
        const int32_t alternative_minimum=alternative_vlc<alternative_fixed?alternative_vlc:alternative_fixed;
        if(current_minimum>alternative_minimum){
            const uint32_t split=split_components(state,count);
            if(split){
                count=split;component_bits=alternative_vlc+(int32_t)count;
                fixed_bits=alternative_fixed+(int32_t)(3*count);common_trim=1;
            }
        }
    }
    if(common_trim){
        const int32_t removed=trim_component_width(state,count);
        if(removed<0)return -2;
        component_bits-=(int32_t)count*removed;
        fixed_bits-=(int32_t)count*removed*3;
    }
    state->tonal_mode=component_bits>fixed_bits;
    if(component_bits>fixed_bits)component_bits=fixed_bits;
    return header_bits+component_bits;
}
