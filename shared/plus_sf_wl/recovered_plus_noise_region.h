#ifndef SONY_RECOVERED_PLUS_NOISE_REGION_H
#define SONY_RECOVERED_PLUS_NOISE_REGION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_noise_region_config {
    uint32_t channels,units,subbands,profile,stereo_flags[16];
} sony_plus_noise_region_config;
typedef struct sony_plus_noise_region_channel {
    float metric[32];int32_t integer_bias[32],word_length[32],scale_factor[32];
    int32_t region[5],active;
} sony_plus_noise_region_channel;
/* Complete original3ec000; typed inputs are the original supplied numeric arrays.
 * subbands=1..16, units=0..32, profile=0..31, channels1..2; metric finite >=0,
 * integer_bias0..32767, WL0..7. Sets all5 outputs to15 before channel activity.
 * Uses original channel borrowing, QU regions, rounding and profile biases.
 * No pointer-shaped Sony state, codec runtime or floating-point host API. */
int sony_recovered_plus_noise_regions(const sony_plus_noise_region_config *config,
    sony_plus_noise_region_channel channels[2]);
#ifdef __cplusplus
}
#endif
#endif
