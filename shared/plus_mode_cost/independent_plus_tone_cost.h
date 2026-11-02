#ifndef INDEPENDENT_PLUS_TONE_COST_H
#define INDEPENDENT_PLUS_TONE_COST_H
#include "../plus_fields/independent_plus_tones.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Logical typed state. The old implementation swaps40-byte records containing
 * a tone-array pointer; this clean representation swaps the corresponding
 * values and previous-envelope words, not allocation addresses. */
typedef struct plus_tone_mode_state {
    uint32_t present,amplitude_mode;
    int32_t band_count,channel_count;
    uint32_t flags[3][18]; /* Existing formatter order310,3a0,358. */
    int32_t stereo_map[256];
    uint32_t previous_envelope[2][16][4];
    plus_given_tone_channel channels[2];
} plus_tone_mode_state;
int32_t plus_tone_prepare_selected(plus_tone_mode_state*,uint32_t channel);
int32_t plus_tone_prepare_stereo_map(plus_tone_mode_state*,uint32_t channel);
/* Candidate costs exclude the fixed channel mode-bit header. Frequency mode0
 * also chooses per-band direction, retaining the original first-on-tie rule.
 * Component choice and channel preparation are separately exposed for tests. */
int32_t plus_tone_mode_candidate(plus_tone_mode_state*,uint32_t channel,plus_tone_substage,uint32_t mode);
int32_t plus_tone_choose_component(plus_tone_mode_state*,uint32_t channel,plus_tone_substage);
int32_t plus_tone_choose_channel(plus_tone_mode_state*,uint32_t channel,int32_t choose);
/* Original3e9a70, including empty-left stereo swaps and flag optimization.
 * choose==1 searches modes; other values force direct modes. Absent tones cost
 * one bit and leave the complete state unchanged. Valid tone counts are0..7. */
int32_t plus_tone_choose_modes(plus_tone_mode_state*,int32_t choose);
#ifdef __cplusplus
}
#endif
#endif
