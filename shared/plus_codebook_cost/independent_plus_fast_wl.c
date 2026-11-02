#include "independent_plus_fast_wl.h"
#include "../plus_fields/independent_plus_quant_fields.h"

/* Independently written from original integer control/data flow. Original
 * executable bytes are test references only. The four immutable Huffman width
 * books are shared with the independently recovered serializer. */
static int32_t add32(int32_t a,int32_t b){
    return (int32_t)((uint32_t)a+(uint32_t)b);
}
static int valid_bounds(const plus_fast_wl_parameters* p){
    return p->total_units>=0&&p->total_units<=32&&
        p->zero_trimmed_units>=0&&p->zero_trimmed_units<=p->total_units&&
        p->one_trimmed_units>=0&&p->one_trimmed_units<=p->total_units;
}
int plus_fast_wl_prepare(const uint32_t w[32],int32_t units,
    uint32_t channel,plus_fast_wl_parameters* p){
    if(!w||!p||units<0||units>32||channel>1)return -1;
    int32_t tail=units;
    const uint32_t value=channel==1?0u:1u;
    while(tail>0&&w[tail-1]==value)--tail;
    p->total_units=units;
    p->zero_trimmed_units=channel==1?tail:units;
    p->one_trimmed_units=channel==1?units:tail;
    return 0;
}

/* Exact 3f0ac0: calculate full/prefix costs with first-table tie behavior. */
static void huffman_pair(const uint32_t symbols[32],int32_t first,
    int32_t total,int32_t cut,int32_t base_bits,int32_t extra_full,
    int32_t extra_prefix,int32_t* full_id,int32_t* prefix_id,
    int32_t* full_cost,int32_t* prefix_cost){
    int32_t all[4]={0,0,0,0},prefix[4]={0,0,0,0};
    for(int32_t i=first;i<total;++i){
        for(uint32_t book=0;book<4;++book){
            const int32_t width=plus_word_length_symbol_width(book,symbols[i]);
            all[book]+=width;
            if(i<cut)prefix[book]+=width;
        }
    }
    *full_id=0;*prefix_id=0;
    for(int32_t book=1;book<4;++book){
        if(all[book]<all[*full_id])*full_id=book;
        if(prefix[book]<prefix[*prefix_id])*prefix_id=book;
    }
    int32_t a=all[*full_id],b=prefix[*prefix_id];
    if(cut>0){a+=base_bits;b+=base_bits;}
    else if(total>0)a+=base_bits;
    *full_cost=a+extra_full;
    *prefix_cost=b+extra_prefix;
}

int32_t plus_fast_wl_candidate(const uint32_t w[32],const uint32_t ref[32],
    plus_fast_wl_parameters* p,uint32_t mode){
    if(!w||!p||!valid_bounds(p)||mode<1||mode>3||(mode!=3&&!ref))return -1;
    uint32_t symbols[32]={0};
    const int32_t total=p->total_units;
    int32_t cut=p->zero_trimmed_units,first=0,base=2;
    if(mode==3){
        for(int32_t i=1;i<total;++i)symbols[i]=(w[i]-w[i-1])&7u;
        first=1;base=5;cut=p->one_trimmed_units;
    }else{
        uint32_t previous=(w[0]-ref[0])&7u;
        symbols[0]=previous;
        for(int32_t i=1;i<total;++i){
            const uint32_t current=(w[i]-ref[i])&7u;
            symbols[i]=mode==1?current:(current-previous)&7u;
            previous=current;
        }
    }
    int32_t full_id,prefix_id,full_cost,prefix_cost;
    huffman_pair(symbols,first,total,cut,base,0,5,
        &full_id,&prefix_id,&full_cost,&prefix_cost);
    plus_fast_wl_method* m=&p->method[mode-1];
    const int trimmed=prefix_cost<full_cost;
    m->range_mode=trimmed?(mode==3?2:1):0;
    m->huffman_id=trimmed?prefix_id:full_id;
    m->encoded_units=trimmed?cut:total;
    return add32(trimmed?prefix_cost:full_cost,mode==3?4:2);
}
static int32_t choose_left(plus_fast_wl_parameters* p){
    p->mode=0;
    if(p->cost[3]<p->cost[0])p->mode=3;
    return p->cost[p->mode];
}
static int32_t choose_right(plus_fast_wl_parameters* p){
    p->mode=0;
    for(int32_t mode=1;mode<3;++mode)
        if(p->cost[mode]<p->cost[p->mode])p->mode=mode;
    return p->cost[p->mode];
}
static void update_right(const uint32_t w[2][32],plus_fast_wl_parameters* p){
    p->cost[1]=plus_fast_wl_candidate(w[1],w[0],p,1);
    p->cost[2]=plus_fast_wl_candidate(w[1],w[0],p,2);
}
int32_t plus_fast_wl_choose(const uint32_t w[2][32],int32_t units,
    int32_t channels,int32_t full,int32_t changed,int32_t enabled,
    plus_fast_wl_parameters p[2]){
    if(!w||!p||units<0||units>32||channels<1||channels>2)return -1;
    int32_t bits=channels*2;
    if(!enabled){
        for(int32_t ch=0;ch<channels;++ch)p[ch].mode=0;
        return bits+3*units*channels;
    }
    if(full){
        plus_fast_wl_prepare(w[0],units,0,&p[0]);
        p[0].cost[0]=units*3;
        p[0].cost[3]=plus_fast_wl_candidate(w[0],0,&p[0],3);
        bits=add32(bits,choose_left(&p[0]));
        if(channels==2){
            plus_fast_wl_prepare(w[1],units,1,&p[1]);
            p[1].cost[0]=units*3;
            update_right(w,&p[1]);
            bits=add32(bits,choose_right(&p[1]));
        }
        return bits;
    }
    if(changed<0||changed>=channels)return -1;
    if(changed==0&&channels==2&&!valid_bounds(&p[1]))return -1;
    if(changed==1&&(p[0].mode<0||p[0].mode>3))return -1;
    plus_fast_wl_prepare(w[changed],units,(uint32_t)changed,&p[changed]);
    if(changed==0){
        p[0].cost[3]=plus_fast_wl_candidate(w[0],0,&p[0],3);
        if(channels==2)update_right(w,&p[1]);
        bits=add32(bits,choose_left(&p[0]));
    }else{
        update_right(w,&p[1]);
        bits=add32(bits,p[0].cost[p[0].mode]);
    }
    if(channels==2)bits=add32(bits,choose_right(&p[1]));
    return bits;
}
