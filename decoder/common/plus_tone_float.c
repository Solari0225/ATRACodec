/* Independently recovered from Sony3f38b0 and3f8ba0..3f9215 math graph.
 * Tables are captured numeric data; no original machine code is used.
 */
#include "plus_tone_float.h"
#include <string.h>
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_MANT_DIG==24 && FLT_RADIX==2,"IEEEbinary32 required");
_Static_assert(sizeof(double)==8 && DBL_MANT_DIG==53,"IEEEbinary64 required");
#include "plus_tone_float_tables.inc"
static float number(uint32_t bits){float f;memcpy(&f,&bits,4);return f;}
static int coded_envelope(const int32_t *e){
    return (!e[4]||(e[6]>=0&&e[6]<=31))&&(!e[5]||(e[7]>=0&&e[7]<=31));
}
int atracodec_plus_tone_envelope(int32_t *a,const int32_t *b){
    if(!a||!b||!coded_envelope(a)||!coded_envelope(b))return -1;
    if(a[4]&&a[6]<a[7]){a[0]=1;a[2]=128+a[6]*4;}
    else if(b[4]){a[0]=1;a[2]=b[6]*4;}
    else{a[0]=0;a[2]=0;}
    if(b[5]&&a[2]<=b[7]*4){a[1]=1;a[3]=b[7]*4;}
    else if(a[5]){a[1]=1;a[3]=128+a[7]*4;}
    else{a[1]=0;a[3]=256;}
    a[3]+=4;if(a[3]>256)a[3]=256;return 0;
}
static int valid_band(const atracodec_plus_tone_band *b,int mode){
    if(!b||b->count>48||(b->count&&!b->notes))return 0;
    const int32_t *e=b->envelope;
    if((e[0]&&(e[2]<0||e[2]>252))||(e[1]&&(e[3]<4||e[3]>256)))return 0;
    for(uint32_t i=0;i<b->count;++i){const atracodec_plus_tone_note *n=b->notes+i;
        if(n->amplitude>=64||n->frequency>=1024||(!mode&&n->fine_amplitude>=16))return 0;}
    return 1;
}
static void render(const atracodec_plus_tone_band *band,unsigned offset,int mode,
    int invert,int channel,int backend,float output[128]){
    memset(output,0,128*sizeof(float));
    for(uint32_t n=0;n<band->count;++n){
        const atracodec_plus_tone_note *note=band->notes+n;
        double amplitude=(double)number(tone_amplitude_bits[note->amplitude]);
        if(!mode)amplitude*=number(tone_fine_bits[note->fine_amplitude]);
        const float rounded=(float)amplitude;
        const int32_t advance=((int32_t)offset-128)*(int32_t)note->frequency;
        uint32_t phase=(((note->phase&31u)<<6)+(uint32_t)advance)&2047u;
        for(unsigned i=0;i<128;++i){
            const float sine=number(tone_sine_bits[phase]);phase=(phase+note->frequency)&2047u;
            if(backend){const float product=rounded*sine;output[i]=output[i]+product;}
            else{const double product=amplitude*(double)sine;output[i]=(float)(product+(double)output[i]);}
        }
    }
    if(channel==1&&invert)for(unsigned i=0;i<128;++i)output[i]=output[i]*-1.0f;
    float envelope[256];for(unsigned i=0;i<256;++i)envelope[i]=1.0f;
    const int32_t *e=band->envelope;
    if(e[0]){
        for(int32_t i=0;i<e[2];++i)envelope[i]=0.0f;
        for(unsigned i=0;i<4;++i)envelope[e[2]+(int32_t)i]=number(tone_envelope_bits[i]);
    }
    if(e[1]){
        for(unsigned i=0;i<4;++i)envelope[e[3]-4+(int32_t)i]=number(tone_envelope_bits[3-i]);
        for(int32_t i=e[3];i<256;++i)envelope[i]=0.0f;
    }
    for(unsigned i=0;i<128;++i)output[i]=output[i]*envelope[offset+i];
}
int atracodec_plus_tone_pair(const atracodec_plus_tone_band *a,int a_mode,int a_invert,
    const atracodec_plus_tone_band *b,int b_mode,int b_invert,float *output,int channel,int backend){
    if(!output||!valid_band(a,a_mode)||!valid_band(b,b_mode))return -1;
    float x[128],y[128];render(a,128,a_mode,a_invert,channel,backend,x);
    render(b,0,b_mode,b_invert,channel,backend,y);
    const int cross=a->count&&b->count&&a->envelope[3]-128>=b->envelope[2];
    const int fade_a=cross||(a->count&&!a->envelope[1]);
    const int fade_b=cross||(b->count&&!b->envelope[0]);
    for(unsigned i=0;i<128;++i){
        if(fade_a)x[i]=x[i]*number(tone_overlap_b_bits[i]);
        if(fade_b)y[i]=y[i]*number(tone_overlap_a_bits[i]);
        output[i]=x[i]+y[i];
    }
    return 0;
}
