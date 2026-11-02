#ifndef SONY_INDEPENDENT_PLUS_PHASE_H
#define SONY_INDEPENDENT_PLUS_PHASE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original3dce00 default SIMD phase compensation. It also independently
 * selects the coherence boundary using exact profile/sample-rate tables.
 * Coefficients are rolling state; input subband samples are in/out. */
int32_t sony_recovered_plus_coherence_boundary(int32_t profile_id,int32_t sample_rate);
int sony_recovered_plus_phase_mix(float left[16][128],float right[16][128],
    float coefficient[16],int32_t *coherence_boundary,int32_t profile_id,
    int32_t sample_rate,uint32_t bands);
#ifdef __cplusplus
}
#endif
#endif
