#ifndef INDEPENDENT_PLUS_COARSE_H
#define INDEPENDENT_PLUS_COARSE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct plus_feature_channel {
    float spectrum[2048];
    int32_t coarse[32];
    float numerator[32],factor[32];
} plus_feature_channel;
typedef struct plus_feature_shared {
    int32_t difference_coarse[32];
    float difference_peak[32];
} plus_feature_shared;
/* Pure indexed numerical constants. Out-of-domain indices return0. */
uint32_t plus_feature_quant_start(uint32_t unit);
uint32_t plus_feature_quant_count(uint32_t unit);
float plus_feature_coarse_scale(uint32_t index);
/* Original4037a0 coarse scale index. Peak is a nonnegative finite float. */
int32_t plus_feature_coarse_index(float peak);
/* Original3ece20/40b4ed: finite spectrum, per-QU absolute peaks and indices.
 * Units0..32; only active output prefixes are changed. */
int plus_feature_coarse(const float spectrum[2048],uint32_t units,
    int32_t coarse[32],float peak[32],uint32_t backend);
/* Original3ecd10/40b46d normalization and3ecda0 high-scale clipping. Coarse
 * indices0..63. A float reciprocal is rounded before each float multiply. */
int plus_feature_normalize(float spectrum[2048],const int32_t coarse[32],uint32_t units);
int plus_feature_clip(float spectrum[2048],const int32_t coarse[32],uint32_t units);
/* Complete3e1030. Spectrum is changed in place. The stereo-difference state
 * is measured before normalization, and mono leaves that shared state intact.
 * Factor[q]=numerator[q]/originalScale[coarse[q]]. Finite active inputs;
 * channels1/2 and units0..32. Backend0/nonzero are original scalar/SSE paths;
 * the SSE all-zero grouped peak's signed zero is preserved. Returns0/-1. */
int plus_feature_prepare_normalized(plus_feature_channel channels[2],
    plus_feature_shared*,uint32_t units,uint32_t channel_count,uint32_t backend);
/* Original3ecec0 demand rounding, with the actual native ordering of the upper
 * limit and lower1 checks. Demand+0.5 must fit signed32 before truncation. */
int plus_feature_round_limits(const float demand[32],const int16_t limit[32],
    uint32_t units,int32_t word_length[32]);
#ifdef __cplusplus
}
#endif
#endif
