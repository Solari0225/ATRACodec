#ifndef SONY_RECOVERED_TRIPLE_DOT_H
#define SONY_RECOVERED_TRIPLE_DOT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original three-pair SIMD reduction. Each lane multiplies and accumulates in
   binary32; the final reduction is (lane0+lane1)+(lane2+lane3). For count>0,
   each input must contain ceil(count/4)*4 elements, matching original reads.
   count<=0 returns three positive zeros without reading the inputs.
   Requires RNE binary32 arithmetic, no FMA, DAZ/FTZ disabled for subnormal data.
   Caller PCM/analysis domains must avoid overflow/NaN if exact NaN payload
   behavior across architectures is required. This is not a full encoder. */
int sony_recovered_triple_dot_f32(const float* a,const float* b,const float* c,
    const float* d,const float* e,const float* f,float output[3],int32_t count);
#ifdef __cplusplus
}
#endif
#endif
