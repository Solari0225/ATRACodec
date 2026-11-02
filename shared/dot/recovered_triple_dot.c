#include "recovered_triple_dot.h"
static float binary32(float value){volatile float rounded=value;return rounded;}
int sony_recovered_triple_dot_f32(const float* a,const float* b,const float* c,
    const float* d,const float* e,const float* f,float output[3],int32_t count){
    if(!output)return -1;
    if(count<=0){output[0]=0.0f;output[1]=0.0f;output[2]=0.0f;return 0;}
    if(!a||!b||!c||!d||!e||!f)return -1;
    float accum[3][4]={{0}};
    unsigned words=((unsigned)count+3u)&~3u;
    for(unsigned i=0;i<words;i+=4)for(unsigned lane=0;lane<4;++lane){
        float p0=binary32(a[i+lane]*b[i+lane]);
        float p1=binary32(c[i+lane]*d[i+lane]);
        float p2=binary32(e[i+lane]*f[i+lane]);
        accum[0][lane]=binary32(accum[0][lane]+p0);
        accum[1][lane]=binary32(accum[1][lane]+p1);
        accum[2][lane]=binary32(accum[2][lane]+p2);
    }
    for(unsigned pair=0;pair<3;++pair){
        float high=binary32(accum[pair][2]+accum[pair][3]);
        float low=binary32(accum[pair][0]+accum[pair][1]);
        output[pair]=binary32(low+high);
    }
    return 0;
}
