#include "recovered_plus_sf_choice.h"
#include "../plus_fields/independent_plus_quant_fields.h"
#include "sf_choice_math.inc"
static int32_t width(uint32_t small,uint32_t table,int32_t symbol){return plus_scale_factor_symbol_width(small,table,(uint32_t)symbol&(small?15u:63u));}
static uint32_t minimum(const int32_t *a,uint32_t n){uint32_t i=0;for(uint32_t j=1;j<n;++j)if(a[j]<a[i])i=j;return i;}
static int valid(const sony_plus_sf_choice_config *g,const sony_plus_sf_choice_channel *c){
    if(!g||!c||g->channels<1||g->channels>2||g->units>32||g->groups>10||g->full_table>1)return -1;
    for(uint32_t ch=0;ch<g->channels;++ch)if(c[ch].channel_index!=ch)return -1;return 0;
}
static void prepare(const sony_plus_sf_choice_config *g,sony_plus_sf_choice_channel *c,int32_t flags[3]){
    const uint32_t n=g->units,groups=g->groups;flags[0]=flags[1]=flags[2]=1;
    for(uint32_t q=0;q<n;++q){
        c->prepared[0][q]=c->coarse[q];
        for(uint32_t j=1;j<3;++j){c->prepared[j][q]=c->coarse[q]+(int32_t)choice_sf_weights[(j-1)*32+q];if(c->prepared[j][q]>=64)flags[j]=0;}
    }
    int32_t delta[10]={0},baseline[10]={0};
    for(uint32_t group=0;group<groups;++group){const uint32_t q=3*group;delta[group]=(int32_t)choice_sf_three_mean[c->coarse[q]+c->coarse[q+1]+c->coarse[q+2]];}
    if(groups==10){int32_t sum=0;for(uint32_t q=27;q<32;++q)sum+=c->coarse[q];delta[9]=(sum>>6)+(sum>>4)+(sum>>3)+1;}
    c->metadata[7]=delta[0];for(uint32_t i=1;i<groups;++i)delta[i]=delta[0]-delta[i];
    const uint32_t count=choice_sf_group_map[n]+1;int32_t shape_cost[64];
    for(uint32_t shape=0;shape<64;++shape){int32_t cost=0;
        for(uint32_t group=1;group<count;++group){int32_t difference=delta[group]-(int8_t)choice_sf_shapes[shape*9+group-1];if(difference<0)difference=-difference;cost+=choice_sf_shape_error[difference];}
        shape_cost[shape]=cost;
    }
    const uint32_t shape=minimum(shape_cost,64);c->metadata[6]=(int32_t)shape;baseline[0]=delta[0];
    for(uint32_t group=1;group<groups;++group)baseline[group]=delta[0]-(int8_t)choice_sf_shapes[shape*9+group-1];
    for(uint32_t group=0;group<groups;++group)for(uint32_t j=0;j<3;++j)c->prepared[3][group*3+j]=c->coarse[group*3+j]-baseline[group];
    if(groups==10)for(uint32_t q=27;q<32;++q)c->prepared[3][q]=c->coarse[q]-baseline[9];
    for(uint32_t q=0;q<n;++q){if(c->prepared[3][q]>=32)c->prepared[3][q]-=64;else if(c->prepared[3][q]<-32)c->prepared[3][q]+=64;}
}
/* Original3eba90, then3eb020's16 fixed bits. */
static int32_t range_cost(const int32_t *v,uint32_t n,int32_t metadata[5]){
    int32_t prefix[7],base[7]={0},active[6]={1,1,1,1,1,1};
    for(uint32_t b=0;b<7;++b)prefix[b]=(int32_t)n;
    int32_t maximum=0,minimum_value=63;
    for(int32_t q=(int32_t)n-1;q>=0;--q){
        if(v[q]>maximum)maximum=v[q];if(v[q]<minimum_value)minimum_value=v[q];
        for(uint32_t b=0;b<6;++b)if(active[b]){
            prefix[b]=q;if(maximum-minimum_value>(int32_t)choice_sf_range_limit[b]){active[b]=0;prefix[b]=q+1;}else base[b]=minimum_value;
        }
    }
    int32_t cost=6*(int32_t)n;uint32_t bits=6;
    for(uint32_t b=0;b<6;++b){const int32_t candidate=6*prefix[b]+(int32_t)b*((int32_t)n-prefix[b]);if(candidate<cost){cost=candidate;bits=b;}}
    metadata[2]=prefix[bits];metadata[3]=(int32_t)bits;metadata[4]=base[bits];return cost+16;
}
/* Original3eb060, zero means no encodable candidate. */
static int32_t compact_shape_range(const int32_t *v,uint32_t n,int32_t metadata[5]){
    int32_t best=16384,best_prefix=0,best_bits=4,best_base=0;int found=0;
    for(uint32_t prefix=0;prefix<n;++prefix){
        int32_t cost=0;int valid_prefix=1;
        for(uint32_t q=0;q<prefix;++q){if(v[q]<-7||v[q]>8){valid_prefix=0;break;}cost+=4;}
        if(!valid_prefix)continue;
        int32_t low=v[prefix],high=low;
        for(uint32_t q=prefix+1;q<n;++q){if(v[q]<low)low=v[q];if(v[q]>high)high=v[q];}
        const int32_t range=high-low;int32_t bits=0;int valid_suffix=low>=-7&&low<=8;
        if(range==0)bits=0;else if(range<2)bits=1;else if(range<4)bits=2;else if(range<8)bits=3;else valid_suffix=0;
        cost+=(int32_t)(n-prefix)*bits;
        if(valid_suffix&&cost<best){found=1;best=cost;best_prefix=(int32_t)prefix;best_bits=bits;best_base=low;}
    }
    if(!found)return 0;metadata[2]=best_prefix;metadata[3]=best_bits;metadata[4]=best_base;return best+25;
}
static int32_t first_range_mode(sony_plus_sf_choice_channel *c,uint32_t n,const int32_t flags[3],int32_t out[5]){
    int32_t cost[4],m[4][5]={{0}};cost[0]=range_cost(c->prepared[0],n,m[0]);
    for(uint32_t j=1;j<3;++j)cost[j]=flags[j]?range_cost(c->prepared[j],n,m[j]):0;
    cost[3]=compact_shape_range(c->prepared[3],n,m[3]);uint32_t best=0;
    for(uint32_t j=1;j<4;++j)if(cost[j]>0&&cost[j]<cost[best])best=j;
    out[1]=(int32_t)best;c->metadata[1]=m[best][2];c->metadata[2]=m[best][3];c->metadata[3]=m[best][4];return cost[best];
}
static int32_t first_shape_huffman(const int32_t *v,uint32_t n,int32_t out[5]){
    int32_t costs[4]={0};out[0]=0;out[1]=3;
    for(uint32_t q=0;q<n;++q){if(v[q]<-7||v[q]>7)return 16384;for(uint32_t t=0;t<4;++t)costs[t]+=width(1,t,v[q]);}
    const uint32_t best=minimum(costs,4);out[0]=(int32_t)best;return costs[best]+14;
}
static int32_t delta_huffman(const int32_t *v,uint32_t n,int32_t out[5]){
    int32_t costs[4]={0};for(uint32_t q=1;q<n;++q)for(uint32_t t=0;t<4;++t)costs[t]+=width(0,t,v[q]-v[q-1]);
    const uint32_t best=minimum(costs,4);out[0]=(int32_t)best;return costs[best]+10;
}
static int32_t shape_delta_huffman(const int32_t *v,uint32_t n,int32_t out[5]){
    int32_t costs[4]={0};out[0]=0;if(v[0]<-8||v[0]>7)return 16384;
    for(uint32_t q=1;q<n;++q){const uint32_t symbol=(uint32_t)(v[q]-v[q-1])&63u;if(symbol>7&&symbol<57)return 16384;
        for(uint32_t t=0;t<4;++t)costs[t]+=width(1,t,(int32_t)symbol);}
    const uint32_t best=minimum(costs,4);out[0]=(int32_t)best;return costs[best]+20;
}
static int32_t first_delta_mode(const sony_plus_sf_choice_channel *c,uint32_t n,const int32_t flags[3],int32_t out[5]){
    int32_t cost[4],m[4][5]={{0}};for(uint32_t j=0;j<3;++j)cost[j]=flags[j]?delta_huffman(c->prepared[j],n,m[j]):16384;
    cost[3]=shape_delta_huffman(c->prepared[3],n,m[3]);const uint32_t best=minimum(cost,4);out[0]=m[best][0];out[1]=(int32_t)best;return cost[best];
}
static int32_t right_huffman(const sony_plus_sf_choice_channel *c,const sony_plus_sf_choice_channel *r,uint32_t n,uint32_t second,int32_t out[5]){
    int32_t costs[4]={0};uint32_t previous=0;
    for(uint32_t q=0;q<n;++q){const uint32_t difference=(uint32_t)(c->coarse[q]-r->coarse[q])&63u,symbol=q&&second?(difference-previous)&63u:difference;
        for(uint32_t t=0;t<4;++t)costs[t]+=width(0,t,(int32_t)symbol);previous=difference;}
    const uint32_t best=minimum(costs,4);out[0]=(int32_t)best;return costs[best]+2;
}
int sony_recovered_plus_choose_scale_factors(const sony_plus_sf_choice_config *g,sony_plus_sf_choice_channel c[2],int32_t enabled,int32_t *bits){
    if(!bits||valid(g,c))return -1;if(!g->units){*bits=0;return 0;}
    if(enabled){if(g->groups<1)return -1;for(uint32_t ch=0;ch<g->channels;++ch)for(uint32_t q=0;q<32;++q)if(c[ch].coarse[q]<0||c[ch].coarse[q]>63)return -1;}
    int32_t total=2*(int32_t)g->channels;
    for(uint32_t ch=0;ch<g->channels;++ch){
        if(!enabled){total+=6*(int32_t)g->units;c[ch].metadata[0]=c[ch].metadata[4]=0;if(ch==0)c[ch].metadata[5]=0;continue;}
        int32_t costs[4]={6*(int32_t)g->units,0,0,0},metadata[4][5]={{0}};
        if(ch==0){int32_t flags[3];prepare(g,c+ch,flags);costs[1]=first_range_mode(c+ch,g->units,flags,metadata[1]);costs[2]=first_shape_huffman(c[ch].prepared[3],g->units,metadata[2]);costs[3]=first_delta_mode(c+ch,g->units,flags,metadata[3]);}
        else{costs[1]=right_huffman(c+ch,c,g->units,0,metadata[1]);costs[2]=right_huffman(c+ch,c,g->units,1,metadata[2]);costs[3]=0;for(uint32_t q=0;q<g->units;++q)if(c[ch].coarse[q]!=c[0].coarse[q]){costs[3]=16384;break;}}
        const uint32_t best=minimum(costs,4);c[ch].metadata[0]=(int32_t)best;c[ch].metadata[4]=metadata[best][0];
        if(ch==0)c[ch].metadata[5]=metadata[best][1];total+=costs[best];
    }
    *bits=total;return 0;
}
int sony_recovered_plus_codebook_class_cost(const sony_plus_sf_choice_config *g,sony_plus_sf_choice_channel c[2],int32_t *bits){
    if(!bits||valid(g,c))return -1;if(!g->units){*bits=0;return 0;}
    int32_t total=4*(int32_t)g->channels+1;
    for(uint32_t ch=0;ch<g->channels;++ch){
        c[ch].book_metadata[0]=0;c[ch].book_metadata[1]=(int32_t)g->units;c[ch].book_metadata[2]=0;
        for(uint32_t q=0;q<32;++q)c[ch].book_class[q]=0;
        for(uint32_t q=0;q<g->units;++q){if(c[ch].word_length[q]>0)c[ch].book_class[q]=1;else if(ch&&c[0].word_length[q]>0)c[ch].book_class[q]=2;
            if(c[ch].book_class[q]==1)total+=choice_class_bit_width[g->full_table];else if(c[ch].book_class[q]==2)++total;}
    }
    *bits=total;return 0;
}
