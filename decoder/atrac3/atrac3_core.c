#include "atrac3_core.h"
#include "atrac3_decoder_tables.h"
#include "atrac3_float_transform.h"
#include "../common/atrac3_float_filter.h"
#include "../common/atrac3_float_pcm16.h"
#include <string.h>

/* Grammar and sequencing reconstructed from the actually executed Sony normal
 * decoder graph467220→467380→467900/467d00/467ae0→4657e0,
 * joint4676b0, and466200. This is independent of the AAL fixed-point graph. */
typedef struct bit_reader {const uint8_t *data;size_t bytes,bit;int bad;} bit_reader;
typedef struct channel_fields {
    float spectrum[1024];
    int32_t gains[4][16];
    uint32_t word[32],sf[32];
    uint32_t gain_bands,active_bands;
} channel_fields;

static uint32_t peek(const bit_reader*r,unsigned n) {
    uint32_t v=0;unsigned j;
    for(j=0;j<n;++j) {size_t p=r->bit+j;v=(v<<1)|(p/8<r->bytes?((r->data[p/8]>>(7-(p&7)))&1u):0u);}
    return v;
}
static uint32_t get(bit_reader*r,unsigned n) {
    uint32_t v;
    if(n>32||r->bit>r->bytes*8||n>r->bytes*8-r->bit){r->bad=1;return 0;}
    v=peek(r,n);r->bit+=n;return v;
}
static int coefficients(bit_reader*r,const atracodec_atrac3_decoder_tables*t,
    unsigned word,unsigned mode,unsigned sf,float*begin,float*end)
{
    const atracodec_atrac3_decoder_book*b;
    if(word<1||word>7||mode>1||sf>63)return -1;
    b=&t->books[mode][word-1];
    while(begin<end) {
        uint32_t code=peek(r,b->bits);int symbol,width;double term;
        if(code>=b->lookup_count)return -1;
        symbol=b->lookup[code];
        if(symbol<0||(unsigned)symbol>=b->width_count||(unsigned)symbol>=b->value_count)return -1;
        width=b->widths[symbol];if(width<1||(unsigned)width>b->bits)return -1;
        get(r,(unsigned)width);if(r->bad)return -1;
        term=(double)t->sf[sf]*(double)b->values[symbol];
        *begin=(float)(term+(double)*begin);++begin;
        if(word==1) {
            if(begin==end||(unsigned)(symbol+1)>=b->value_count)return -1;
            term=(double)t->sf[sf]*(double)b->values[symbol+1];
            *begin=(float)(term+(double)*begin);++begin;
        }
    }
    return 0;
}
static int parse_channel(const uint8_t*data,size_t bytes,int joint_right,
    channel_fields*f,const atracodec_atrac3_decoder_tables*t)
{
    bit_reader r={data,bytes,0,0};unsigned maximum=0,band,groups,tonal_mode=0,components=0,q,units,mode;
    memset(f,0,sizeof(*f));
    if(joint_right) {
        if(bytes<2||(data[1]&12)!=12)return -1;
        f->gain_bands=(data[1]&3)+1;r.bit=16;
    } else {
        if(get(&r,6)!=0x28)return -1;
        f->gain_bands=get(&r,2)+1;
    }
    for(band=0;band<4;++band) {
        unsigned points=band<f->gain_bands?get(&r,3):0,j,previous=0;
        for(j=0;j<points;++j) {
            unsigned level=get(&r,4),position=get(&r,5)*8;
            if(j&&position<=previous)return -1;
            f->gains[band][2*j]=(int32_t)position;f->gains[band][2*j+1]=(int32_t)level;previous=position;
        }
        f->gains[band][2*points]=255;f->gains[band][2*points+1]=4;
        if(points)maximum=(band+1)*256;
    }
    groups=get(&r,5);if(groups)tonal_mode=get(&r,2);if(tonal_mode==2)return -1;
    for(q=0;q<groups;++q) {
        unsigned flags[4]={0,0,0,0},count,word,quarter;
        for(band=0;band<f->gain_bands;++band)flags[band]=get(&r,1);
        count=get(&r,3)+1;word=get(&r,3);if(word<2)return -1;
        mode=tonal_mode<2?tonal_mode:get(&r,1);
        for(quarter=0;quarter<f->gain_bands*4;++quarter) {
            unsigned n,j;if(!flags[quarter/4])continue;n=get(&r,3);
            for(j=0;j<n;++j) {
                unsigned sf,position,end;if(components++==64)return -1;
                sf=get(&r,6);position=quarter*64+get(&r,6);end=position+count;if(end>1024)end=1024;
                if(maximum<end)maximum=end;
                if(coefficients(&r,t,word,mode,sf,f->spectrum+position,f->spectrum+end))return -1;
            }
        }
    }
    units=get(&r,5)+1;mode=get(&r,1);
    for(q=0;q<units;++q)f->word[q]=get(&r,3);
    for(q=0;q<units;++q)if(f->word[q])f->sf[q]=get(&r,6);
    for(q=0;q<units;++q)if(f->word[q])
        if(coefficients(&r,t,f->word[q],mode,f->sf[q],f->spectrum+t->boundaries[q],f->spectrum+t->boundaries[q+1]))return -1;
    if(maximum<t->boundaries[units])maximum=t->boundaries[units];
    f->active_bands=(maximum+255)/256;
    return r.bad||f->active_bands>4?-1:0;
}
static void synthesize(atracodec_atrac3_channel_state*s,const channel_fields*f)
{
    uint32_t bands=f->active_bands>s->active_bands?f->active_bands:s->active_bands;
    int32_t first[4]={4,4,4,4};unsigned band,i;
    s->active_bands=f->active_bands;
    memmove(s->signal,s->signal+1024,138*sizeof(float));
    for(band=0;band<bands;++band)first[band]=f->gains[band][1];
    atracodec_atrac3_float_transform(s->signal,s->overlap,s->gain_history,bands,first,f->spectrum,atracodec_atrac3_float_transform_default_tables());
    for(band=bands;band<4;++band) {
        memset(s->overlap[band],0,sizeof(s->overlap[band]));
        for(i=0;i<256;++i)s->signal[138+4*i+band]=0.0f;
    }
    memcpy(s->gain_history,f->gains,sizeof(s->gain_history));
}
int atracodec_atrac3_core_initialize(atracodec_atrac3_core_state*s,uint32_t bitrate)
{
    unsigned ch,band;
    if(!s)return -1;memset(s,0,sizeof(*s));
    switch(bitrate){case 66000:s->frame_bytes=192;s->joint_stereo=1;break;case 105000:s->frame_bytes=304;break;case 132000:s->frame_bytes=384;break;default:return -1;}
    s->bitrate=bitrate;
    for(ch=0;ch<2;++ch)for(band=0;band<4;++band){s->channel[ch].gain_history[band][0]=255;s->channel[ch].gain_history[band][1]=4;}
    for(band=0;band<4;++band)s->joint.matrix_modes[band]=s->joint.previous_matrix_modes[band]=3;
    s->joint.threshold=s->joint.previous_threshold=4;
    return 0;
}
int atracodec_atrac3_core_decode(atracodec_atrac3_core_state*s,const uint8_t*core,size_t bytes,int16_t out[2048])
{
    channel_fields fields;uint8_t reverse[192];const atracodec_atrac3_decoder_tables*t;
    unsigned i;if(!s||!core||!out||bytes!=s->frame_bytes)return -1;
    t=atracodec_atrac3_decoder_default_tables();
    if(parse_channel(core,s->joint_stereo?bytes:bytes/2,0,&fields,t))return -1;
    synthesize(&s->channel[0],&fields);
    if(s->joint_stereo) {
        uint32_t packed;
        for(i=0;i<192;++i)reverse[i]=core[191-i];
        if(parse_channel(reverse,192,1,&fields,t))return -1;
        synthesize(&s->channel[1],&fields);
        atracodec_atrac3_joint_synthesize(s->channel[0].signal,s->channel[1].signal,&s->joint,atracodec_atrac3_joint_default_tables());
        for(i=0;i<4;++i)s->joint.previous_matrix_modes[i]=s->joint.matrix_modes[i];
        s->joint.previous_weight_mode=s->joint.weight_mode;s->joint.previous_threshold=s->joint.threshold;
        s->joint.weight_mode=reverse[0]>>4;s->joint.threshold=reverse[1]&3;
        packed=((uint32_t)reverse[0]<<4)|(reverse[1]>>4);
        for(i=4;i>0;--i){uint32_t mode=packed&3;if(mode==2)return -1;s->joint.matrix_modes[i-1]=mode;packed>>=2;}
    } else {
        if(parse_channel(core+bytes/2,bytes/2,0,&fields,t))return -1;
        synthesize(&s->channel[1],&fields);
    }
    if(atracodec_atrac3_float_filter(s->channel[0].signal,1162)||atracodec_atrac3_float_filter(s->channel[1].signal,1162))return -1;
    return atracodec_atrac3_float_pcm16(s->channel[0].signal,s->channel[1].signal,out);
}
