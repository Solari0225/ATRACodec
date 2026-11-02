#include "independent_atrac3_qmf.h"
#include <stdint.h>
#include <string.h>
#include "qmf_math_bits.inc"

/* Explicit binary32 sequence reconstructed from the active SSE kernel.
 * These are arithmetic formulas and indexed numerical constants, not embedded
 * original code. Volatile stores fix the rounding point of every operation.
 */
static float f32_add(float a,float b){volatile float r=a+b;return r;}
static float f32_mul(float a,float b){volatile float r=a*b;return r;}
static float from_bits(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static float xor_bits(float f,uint32_t mask){uint32_t u;memcpy(&u,&f,4);u^=mask;return from_bits(u);}

void independent_atrac3_qmf(const float input[1024],float history[138],float output[1024]){
    float scratch[1162],first_taps[48],second_taps[96];
    memcpy(scratch,history,138*sizeof(float));
    memcpy(scratch+138,input,1024*sizeof(float));
    for(unsigned i=0;i<48;i++)first_taps[i]=from_bits(qmf_math_bits[4+i]);
    for(unsigned i=0;i<96;i++)second_taps[i]=from_bits(qmf_math_bits[52+i]);
    for(unsigned block=0;block<256;block++){
        const unsigned pos=block*4;
        float phase_a[4],phase_b[4],mixed[4],stage1[4],stage2[4];
        for(unsigned lane=0;lane<4;lane++){
            float a=f32_mul(first_taps[44+lane],scratch[pos+136+lane]);
            float b=f32_mul(first_taps[44+lane],scratch[pos+138+lane]);
            for(unsigned tap=0;tap<11;tap++){
                a=f32_add(a,f32_mul(first_taps[tap*4+lane],scratch[pos+92+tap*4+lane]));
                b=f32_add(b,f32_mul(first_taps[tap*4+lane],scratch[pos+94+tap*4+lane]));
            }
            phase_a[lane]=a;phase_b[lane]=b;
        }
        mixed[0]=f32_add(phase_a[2],phase_a[0]);
        mixed[1]=f32_add(phase_b[2],phase_b[0]);
        mixed[2]=f32_add(phase_a[3],phase_a[1]);
        mixed[3]=f32_add(phase_b[3],phase_b[1]);
        for(unsigned lane=0;lane<4;lane++)
            stage1[lane]=f32_add(xor_bits(mixed[(lane+2)&3],qmf_math_bits[lane]),mixed[lane]);
        for(unsigned lane=0;lane<4;lane++){
            float sum=f32_mul(second_taps[92+lane],stage1[lane]);
            for(unsigned tap=0;tap<23;tap++)
                sum=f32_add(sum,f32_mul(second_taps[tap*4+lane],scratch[pos+tap*4+lane]));
            stage2[lane]=sum;
        }
        /* Original shuffles b1 then e1, with sign-bit XOR between them. */
        output[pos+0]=f32_add(stage2[0],xor_bits(stage2[1],qmf_math_bits[149]));
        output[pos+1]=f32_add(stage2[1],xor_bits(stage2[0],qmf_math_bits[148]));
        output[pos+2]=f32_add(stage2[3],xor_bits(stage2[2],qmf_math_bits[150]));
        output[pos+3]=f32_add(stage2[2],xor_bits(stage2[3],qmf_math_bits[151]));
        memcpy(scratch+pos+92,stage1,4*sizeof(float));
    }
    memcpy(history,scratch+1024,138*sizeof(float));
}
