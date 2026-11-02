#ifndef ATRACODEC_PLUS_NOISE_FLOAT_H
#define ATRACODEC_PLUS_NOISE_FLOAT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Optional normal-decoder time-domain noise, original3f10c0.
 * One call modifies16bands of128samples. level is0..15; start_index is
 * frame noise_index+channel*16, hence0..31 for mono/stereo.
 * Returns0 or-1 for invalid input without a write. */
int atracodec_plus_add_noise_float(float *const bands[16],uint32_t level,uint32_t start_index);
#ifdef __cplusplus
}
#endif
#endif
