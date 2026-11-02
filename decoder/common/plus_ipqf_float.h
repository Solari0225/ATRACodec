#ifndef ATRACODEC_PLUS_IPQF_FLOAT_H
#define ATRACODEC_PLUS_IPQF_FLOAT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct atracodec_plus_ipqf_float_state {
    float delay[576];
    int32_t ring;
} atracodec_plus_ipqf_float_state;
/* Internal Sony-derived ordinaryATRAC3plus synthesis. Each of16 input
 * bands contains128float32 values; output contains2048float32 values.
 * Zero-initialize the state before the stream. Buffers must be distinct.
 * IEEE nearest rounding, gradual underflow, no FMA contraction required.
 * Returns0; null inputs or ring outside0..3 return-1 without a write.
 * This is a synthesis module, not a full decoder or public C ABI.
 */
int atracodec_plus_ipqf_float(const float *const bands[16],float *output,
    atracodec_plus_ipqf_float_state *state);
#ifdef __cplusplus
}
#endif
#endif
