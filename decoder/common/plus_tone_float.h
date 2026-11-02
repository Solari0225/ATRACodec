#ifndef ATRACODEC_PLUS_TONE_FLOAT_H
#define ATRACODEC_PLUS_TONE_FLOAT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct atracodec_plus_tone_note {
    uint32_t amplitude,fine_amplitude,phase,frequency;
} atracodec_plus_tone_note;
typedef struct atracodec_plus_tone_band {
    /* Effective start/stop flags+sample offsets, then coded start/stop
     * flags+positions. This explicit math representation holds no nativeptr. */
    int32_t envelope[8];
    uint32_t count;
    const atracodec_plus_tone_note *notes;
} atracodec_plus_tone_band;
/* Update current effective envelope from current/previous coded envelopes.
 * Native equivalent3f38b0; current[0..3] change, previous remains intact. */
int atracodec_plus_tone_envelope(int32_t current[8],const int32_t previous[8]);
/* Render one band from two adjacent slots. Envelopes must already have been
 * updated. amplitude_mode0 uses fine amplitude, nonzero uses coarse only.
 * channel1 applies requested inversion. backend nonzero reproduces original
 * SIMD rounding; backend0 reproduces original scalar binary64 intermediates.
 * Each notes array has count entries (0..48); output has128float32 samples.
 * Requires nearest IEEE rounding, gradual underflow, no FMA contraction.
 * Returns0 or-1 for invalid math input. Not public codec/container ABI. */
int atracodec_plus_tone_pair(const atracodec_plus_tone_band *a,int a_mode,int a_invert,
    const atracodec_plus_tone_band *b,int b_mode,int b_invert,float output[128],
    int channel,int backend);
#ifdef __cplusplus
}
#endif
#endif
