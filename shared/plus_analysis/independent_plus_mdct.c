#include "independent_plus_mdct.h"
#include <string.h>
#include "mdct_tables.inc"
static float add(float a,float b){volatile float v=a+b;return v;}
static float sub(float a,float b){volatile float v=a-b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static float number(uint32_t word){float v;memcpy(&v,&word,4);return v;}
static float negate(float value){uint32_t word;memcpy(&word,&value,4);word^=0x80000000u;return number(word);}
int sony_recovered_plus_mdct(float state[192],float output[128],
    uint32_t window_flag,float gain_scale,uint32_t reverse_flag)
{
    if(!state||!output)return -1;
    const uint32_t *window=window_flag?mdct_window1:mdct_window0;
    float temporary[128];
    for(uint32_t i=0;i<64;++i){
        const uint32_t q=(i>>2)*16+(i&3);
        const float a=state[64+i],b=state[191-i];
        const float folded=sub(negate(mul(b,number(window[q]))),
            mul(a,number(window[q+8])));
        const float overlap=mul(state[i],gain_scale);
        state[i]=sub(mul(a,number(window[q+12])),mul(b,number(window[q+4])));
        const float odd=mul(sub(folded,overlap),number(mdct_prefold[i]));
        const uint32_t position=2*mdct_permutation[i];
        temporary[position]=add(odd,add(folded,overlap));
        temporary[position+1]=odd;
    }
    for(uint32_t i=0;i<32;++i){
        const uint32_t p=4*i;
        const float a=temporary[p],b=temporary[p+1];
        const float c=temporary[p+2],d=temporary[p+3];
        const float odd=mul(sub(b,d),number(mdct_initial[i]));
        const float even=mul(sub(a,c),number(mdct_initial[i]));
        temporary[p]=add(add(a,c),odd);
        temporary[p+1]=add(add(b,d),even);
        temporary[p+2]=even;temporary[p+3]=odd;
    }
    uint32_t coefficient=0;
    for(uint32_t width=4;width<=32;width*=2){
        for(uint32_t block=0;block<128;block+=2*width){
            const float scale=number(mdct_stage[coefficient++]);
            for(uint32_t i=0;i<width;++i){
                const float a=temporary[block+i],b=temporary[block+width+i];
                temporary[block+i]=add(a,b);
                temporary[block+width+i]=mul(sub(a,b),scale);
            }
            for(uint32_t i=0;i<width;++i)
                temporary[block+i]=add(temporary[block+i],
                    temporary[block+2*width-1-i]);
        }
    }
    const float last_scale=number(0x3f3504f3u);
    for(uint32_t i=0;i<64;++i){
        const float a=temporary[i],b=temporary[i+64];
        output[i]=add(a,b);output[i+64]=mul(sub(a,b),last_scale);
    }
    for(uint32_t i=0;i<64;++i){
        const float high=output[127-i];
        if(reverse_flag){output[127-i]=add(high,output[i]);output[i]=high;}
        else output[i]=add(output[i],high);
    }
    return 0;
}
