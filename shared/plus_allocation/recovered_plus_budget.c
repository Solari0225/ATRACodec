#include "recovered_plus_budget.h"
#include "../plus_quantization/recovered_plus_quantization.h"

static const uint16_t qu_start[33]={0,16,32,48,64,80,96,112,128,160,192,224,256,288,320,352,384,448,512,576,640,704,768,896,1024,1152,1280,1408,1536,1664,1792,1920,2048};
int sony_recovered_plus_priority_sort(int32_t *scores,int32_t *indices,uint32_t count){
    if(!scores||!indices||count>64)return -1;
    for(uint32_t i=0;i<count;++i)indices[i]=(int32_t)i;
    uint32_t gap=1;while(gap<=count)gap=gap*3+1;
    for(gap/=3;gap>=1;gap/=3){
        for(uint32_t i=gap;i<count;++i){
            const int32_t value=scores[i];int32_t j=(int32_t)i-(int32_t)gap;
            while(j>=0&&scores[j]<value){
                scores[j+gap]=scores[j];
                const int32_t saved=indices[j+gap];indices[j+gap]=indices[j];indices[j]=saved;
                j-=(int32_t)gap;
            }
            scores[j+(int32_t)gap]=value;
        }
    }
    return 0;
}
int sony_recovered_plus_priorities(uint32_t channels,uint32_t units,int32_t special,
    const sony_plus_priority_channel input[2],int32_t *scores,int32_t *indices){
    if(!input||!scores||!indices||channels<1||channels>2||units>32)return -1;
    for(uint32_t ch=0;ch<channels;++ch){
        int32_t *out=scores+ch*units;
        for(uint32_t q=0;q<units;++q){
            if(special)out[q]=input[ch].coarse[q];
            else{
                const float p=input[ch].peakiness[q];
                if(!(p>=-1048576.0f&&p<=1048576.0f)||input[ch].coarse[q]<0||input[ch].coarse[q]>63)return -1;
                int32_t base=input[ch].coarse[q]-(int32_t)((q+4)>>3);
                if(base<0)base=0;out[q]=base+(int32_t)p;
            }
        }
        if(!special)for(uint32_t q=12;q<units;++q){
            const int32_t high=input[ch].coarse[q];int32_t difference=63;uint32_t j=1;
            for(;j<=4;++j){const int32_t low=input[ch].coarse[q-j];if(high<low)break;if(difference>high-low)difference=high-low;}
            if(j==5&&difference>3)out[q]+=difference/2;
        }
    }
    return sony_recovered_plus_priority_sort(scores,indices,channels*units);
}
int sony_recovered_plus_trial_cost(uint32_t unit,uint32_t wl,int32_t current_book,
    const sony_plus_unit_cost_input *in,const sony_plus_cost_matrix *current,
    sony_plus_cost_matrix *candidate,int32_t *selected,int32_t *difference){
    if(!in||!current||!candidate||!selected||!difference||unit>31||wl<1||wl>7||current_book<0||current_book>3)return -1;
    if(sony_recovered_plus_quantization_cost(in->bank,wl,(uint32_t)in->threshold_index[unit],in->threshold_factor[unit],
        qu_start[unit+1]-qu_start[unit],in->normalized+qu_start[unit],in->simd,candidate->costs[unit]))return -1;
    int32_t best=current_book;int16_t minimum=0x4000;
    for(int32_t i=0;i<4;++i)if(candidate->costs[unit][i]<minimum){minimum=candidate->costs[unit][i];best=i;}
    *selected=best;
    *difference=(int16_t)(uint16_t)(candidate->costs[unit][best]-current->costs[unit][current_book]);
    return 0;
}
