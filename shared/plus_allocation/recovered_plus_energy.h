#ifndef SONY_RECOVERED_PLUS_ENERGY_H
#define SONY_RECOVERED_PLUS_ENERGY_H
#include <stdint.h>
#include "../plus_analysis/independent_plus_gain_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_energy_noise_config {uint32_t channels,stereo_band[16];} sony_plus_energy_noise_config;
typedef struct sony_plus_energy_noise_channel {
    sony_plus_gain_points current[16],previous[16];int32_t region[5];
} sony_plus_energy_noise_channel;
typedef struct sony_plus_energy_noise_cache {int32_t band;float noise[128];} sony_plus_energy_noise_cache;
/* Complete original403290: seed each active subband from the wrapped sum of
 * active scale-factor indices across channels. Inactive seeds are untouched. */
int sony_recovered_plus_noise_seeds(uint32_t channels,uint32_t units,uint32_t subbands,
    const int32_t scales[2][32],uint16_t seeds[16]);
/* Complete original403550, including signed16 comparisons. Locations/window
 * flags are unused by this mathematical leaf; valid level indices0..15. */
int sony_recovered_plus_noise_gain(const sony_plus_gain_points *first,
    const sony_plus_gain_points *second,int32_t *exponent);
/* Original3ee490 scalar and40b9fd SIMD branches. Value=n*factor/sum(abs(x));
 * scalar quarter ordering and SIMD binary32 lane reduction are preserved.
 * Returns1 for nonzero sum,0 for all-zero input,-1 for invalid input. */
int sony_recovered_plus_absolute_ratio(const float *values,float factor,
    uint32_t count,uint32_t simd,float *output);
/* Complete original403340 deterministic noise addition, including channel
 * borrowing, same-band cache reuse, gain exponent and scalar/SIMD rounding.
 * Values are already quantized int16 converted to floats, in/out. The caller
 * invokes this only for channels whose noise-presence flag is enabled. */
int sony_recovered_plus_add_noise(const sony_plus_energy_noise_config *config,
    const sony_plus_energy_noise_channel channels[2],uint16_t seed,
    uint32_t channel,uint32_t unit,uint32_t word_length,float values[128],
    sony_plus_energy_noise_cache *cache,uint32_t simd);
/* Complete original3e81e0: minimum zero-spectrum representation cost. */
int sony_recovered_plus_minimum_spectrum_cost(uint32_t channels,uint32_t units,
    const uint32_t banks[2],const int32_t words[2][32],int32_t *bits);
#ifdef __cplusplus
}
#endif
#endif
