#ifndef ATRACODEC_PLUS_POWER_FLOAT_H
#define ATRACODEC_PLUS_POWER_FLOAT_H
#include <stdint.h>
#include "../plus/plus_gain_float.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Explicit ordinary ATRAC3plus power-compensation inputs. The caller selects
 * reference gain/power fields using the native stereo-swap flag; WL/SF always
 * belong to the target channel. Native gain locations/window are unused here. */
typedef struct atracodec_plus_power_config {
    int32_t target_word_length[32],target_scale_factor[32];
    atracodec_plus_gain reference_current_gain,reference_previous_gain;
    uint32_t band,power_level;
    uint16_t seed16,reserved;
} atracodec_plus_power_config;

/* Complete original4035e0 mathematical effect on the2048float spectrum.
 * Valid band0..15, power0..15, seed0..1020, active WL1..7 and SF0..63.
 * WL<=0 is inactive. Power15 bypasses all other unused input fields.
 * Returns0 on success/bypass,-1 on invalid active fields without a write.
 * Requires IEEE nearest-even, gradual underflow and disabled FMA contraction. */
int atracodec_plus_apply_power_float(const atracodec_plus_power_config*,float spectrum[2048]);

#ifdef __cplusplus
}
#endif
#endif
