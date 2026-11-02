#include "atrac3_float_transform.h"

/* Independent scalar reconstruction of the original routine at image+4657e0.
 * The operation order follows the actually executed SSE/x87 code, rather than
 * a textbook IMDCT identity. No executable fragment is linked or interpreted. */
void atracodec_atrac3_float_transform(
    float signal[1162], float overlap[4][128],
    const int32_t gain_history[4][16], unsigned bands,
    const int32_t first_gain[4], const float spectrum[1024],
    const atracodec_atrac3_float_transform_tables *t)
{
    float w[1536], gain[4] = {1.0f,1.0f,1.0f,1.0f};
    unsigned i, lane;
    int step;
    for (lane=0; lane<bands && lane<4; ++lane)
        gain[lane]=t->scale[(unsigned)first_gain[lane]&15u];
    for (i=0;i<256;++i) {
        w[512+4*i]=spectrum[i];
        w[512+4*i+2]=spectrum[512+i];
        w[1533-4*i]=spectrum[256+i];
        w[1535-4*i]=spectrum[768+i];
    }
    for (i=0;i<128;++i)
        for (lane=0;lane<4;++lane) w[4*i+lane]=overlap[lane][i];

    for (i=0;i<64;++i) {
        unsigned a=1020-4*i,b=1024+4*i;
        for (lane=0;lane<4;++lane) {
            float av=w[a+lane],bv=w[b+lane];
            float cv=w[b-512+lane],dv=w[a+512+lane];
            float x=av-bv,y=cv-dv;
            float p=bv*t->butterfly[63],q=dv*t->butterfly[63];
            w[b-512+lane]=y+p;
            w[b+lane]=y-p;
            w[a+512+lane]=x-q;
            w[a+lane]=x+q;
        }
    }
    for (step=64;step>=2;step/=2) {
        int segment;
        int base=512;
        int coef=127-step/2;
        for (segment=step/2;segment<128;segment+=step) {
            int count=(step*2-1)/4+1;
            int p=base,distance=step*8;
            int j;
            for(j=0;j<count;++j) {
                for(lane=0;lane<4;++lane) {
                    w[p+lane]=w[p+lane]-w[p+distance-4+lane];
                    w[p+4+lane]=w[p+4+lane]-w[p+distance-8+lane];
                }
                p+=8;distance-=16;
            }
            for(j=base;j<p;j+=8) {
                int offset=(step+1)*4;
                float k=t->butterfly[coef];
                for(lane=0;lane<4;++lane) {
                    float x=w[j+offset-4+lane]*k;
                    float y=w[j+offset+lane]*k;
                    float a=w[j+lane],b=w[j+4+lane];
                    w[j+offset-4+lane]=a-x;
                    w[j+offset+lane]=b-y;
                    w[j+lane]=a+x;
                    w[j+4+lane]=b+y;
                }
            }
            base+=step*8;
            coef-=step;
        }
    }
    for(i=0;i<128;++i) {
        unsigned p=(unsigned)t->permutation[127-i];
        unsigned a=1532-8*i;
        for(lane=0;lane<4;++lane) {
            float last=w[a+lane];
            float difference=w[a-4+lane]-last;
            float rotated=last*t->rotation[p][3];
            float value=(difference-rotated)*gain[lane];
            float old=w[4*p+lane];
            float product;
            value=value*t->rotation[p][2];
            product=value*t->rotation[p][0];
            signal[138+4*(127-p)+lane]=old-product;
            product=old*t->rotation[p][0];
            signal[138+4*(128+p)+lane]=product+value;
            value=difference+rotated;
            w[4*p+lane]=value*t->rotation[p][1];
        }
    }
    for(i=0;i<128;++i)
        for(lane=0;lane<4;++lane) overlap[lane][i]=w[4*i+lane];

    /* x87 CW=027f: multiply in binary64, round only at the original float store. */
    for(lane=0;lane<(bands?bands:1) && lane<4;++lane) {
        unsigned point=0;
        int sample=0;
        const int32_t *g=gain_history[lane];
        for (;;) {
            int boundary=g[point]-255+256;
            int level=g[point+1];
            if(level==4) {
                sample=boundary;
                if(sample>=256) break;
            } else {
                float factor=t->scale[(unsigned)level&15u];
                signal[138+4*sample+lane]=factor*signal[138+4*sample+lane];
                ++sample;
                while(sample<boundary) {
                    for(i=0;i<8;++i) {
                        signal[138+4*(sample+(int)i)+lane]=
                            factor*signal[138+4*(sample+(int)i)+lane];
                    }
                    sample+=8;
                }
            }
            point+=2;
            {
                int delta=g[point+1]-level;
                int exponent=level*8+delta;
                for(i=0;i<7;++i) {
                    unsigned e=(unsigned)exponent;
                    double factor=(double)t->scale[e>>3]*(double)t->fraction[e&7u];
                    unsigned pos=138+4*(unsigned)(sample+(int)i)+lane;
                    signal[pos]=(float)(factor*(double)signal[pos]);
                    exponent+=delta;
                }
                sample+=7;
            }
            if(sample>=256) break;
        }
    }
}


