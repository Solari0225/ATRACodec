#ifndef ATRACODEC_PLUS_CORE_H
#define ATRACODEC_PLUS_CORE_H
#include "plus_core_reader.h"
#include "../common/plus_ipqf_float.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct atracodec_plus_core_state {
    float overlap[2][16][64];
    atracodec_plus_ipqf_float_state synthesis[2];
    atracodec_plus_gain previous_gain[2][16];
    atracodec_plus_tone_fields previous_tones;
} atracodec_plus_core_state;
void atracodec_plus_core_reset(atracodec_plus_core_state *state);
/* Internal normal Sony decoder composition. One physical frame produces2048
 * planar float samples per channel; container priming/crop is applied later.
 * Default Sony backend semantics are portable and do not require SIMD CPU.
 * Returns0 or-1 for invalid parsed math input. */
int atracodec_plus_core_render(const atracodec_plus_frame *frame,
    atracodec_plus_core_state *state,float output[2][2048]);
#ifdef __cplusplus
}
#endif
#endif
