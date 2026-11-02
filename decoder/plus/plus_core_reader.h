#ifndef ATRACODEC_PLUS_CORE_READER_H
#define ATRACODEC_PLUS_CORE_READER_H
#include <stdint.h>
#include "plus_gain_float.h"
#include "../common/plus_tone_float.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct atracodec_plus_tone_fields_band {
    int32_t envelope[8];
    uint32_t count,first_note;
} atracodec_plus_tone_fields_band;
typedef struct atracodec_plus_tone_fields {
    uint32_t present,amplitude_mode,bands,note_count;
    uint32_t shared[16],swap[16],invert[16];
    atracodec_plus_tone_note notes[48];
    atracodec_plus_tone_fields_band band[2][16];
} atracodec_plus_tone_fields;
typedef struct atracodec_plus_frame {
    uint32_t channels,units,frame_subbands,coefficient_subbands,mute;
    int32_t word_length[2][32],scale_factor[2][32],codebook[2][32];
    int16_t quantized[2][2048];
    uint32_t negative[16],swap[16],window[2][16];
    atracodec_plus_gain gain[2][16];
    uint32_t power_level[2][5];
    atracodec_plus_tone_fields tones;
    uint32_t noise_present,noise_level,noise_index;
} atracodec_plus_frame;

/* Parse one physical ordinary mono/stereo frame, including unit2 extension
 * payloads before/after its single audio unit. Bit positions mark header, WL,
 * SF, books, spectrum/PWC/stereo, windows, gain, tones, noise and final unit.
 * Returns0, or a negative stage for malformed/truncated syntax. */
int atracodec_plus_read_core(const uint8_t *data,uint32_t bytes,
                           atracodec_plus_frame *out,uint32_t positions[10]);
#ifdef __cplusplus
}
#endif
#endif
