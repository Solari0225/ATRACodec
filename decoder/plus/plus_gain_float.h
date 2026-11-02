#ifndef ATRACODEC_PLUS_GAIN_FLOAT_H
#define ATRACODEC_PLUS_GAIN_FLOAT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct atracodec_plus_gain {
    uint32_t count;
    uint32_t locations[7];
    uint32_t levels[7];
    uint32_t window_flag;
} atracodec_plus_gain;

/* Internal Sony ordinary decoder gain envelope. Returns-1 for invalid syntax
 * fields without a write,0 otherwise. Tail beyond the final transition is
 * retained, as in the original. IEEE nearest, gradual underflow, no FMA. */
int atracodec_plus_apply_gain_float(const atracodec_plus_gain *gain,float samples[128]);
/* Initial inverse-transform scalar from the first gain level. */
float atracodec_plus_gain_scale_float(const atracodec_plus_gain *gain);
#ifdef __cplusplus
}
#endif
#endif
