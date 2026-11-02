#ifndef INDEPENDENT_PLUS_TONES_H
#define INDEPENDENT_PLUS_TONES_H
#include "independent_plus_flags.h"
#ifdef __cplusplus
extern "C" {
#endif
/* All modes, selected-band flags, quantized tone parameters and stereo maps
 * are inputs already decided by the original analysis stage. */
typedef struct plus_given_tone {uint32_t amplitude,fine_amplitude,phase,frequency;} plus_given_tone;
typedef struct plus_given_tone_band {
    uint32_t envelope[4]; /* Start/end presence, then start/end locations. */
    uint32_t count;
    plus_given_tone tones[16];
} plus_given_tone_band;
typedef struct plus_given_tone_channel {
    uint32_t channel_index,envelope_mode,count_mode,frequency_mode,amplitude_mode,fine_amplitude_mode;
    uint32_t frequency_direction[16],selected[16];
    plus_given_tone_band bands[16];
} plus_given_tone_channel;
typedef struct plus_given_tone_global {
    uint32_t present,amplitude_mode;
    int32_t band_count;
    plus_given_flag_vector stereo_flags[3]; /* Original order310,3a0,358. */
    int32_t stereo_map[256]; /* Packed order of tones in selected bands. */
} plus_given_tone_global;
typedef enum plus_tone_substage {
    PLUS_TONE_ENVELOPE,PLUS_TONE_COUNT,PLUS_TONE_FREQUENCY,
    PLUS_TONE_AMPLITUDE,PLUS_TONE_FINE_AMPLITUDE,PLUS_TONE_PHASE
} plus_tone_substage;
int32_t plus_pack_tone_substage(plus_bit_writer*,const plus_given_tone_global*,
    const plus_given_tone_channel*,const plus_given_tone_channel* reference,
    plus_tone_substage);
int32_t plus_pack_tones(plus_bit_writer*,const plus_given_tone_global*,
    const plus_given_tone_channel* channels,int32_t channel_count);
#ifdef __cplusplus
}
#endif
#endif
