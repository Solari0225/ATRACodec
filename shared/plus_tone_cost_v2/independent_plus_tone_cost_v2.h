#ifndef INDEPENDENT_PLUS_TONE_COST_V2_H
#define INDEPENDENT_PLUS_TONE_COST_V2_H
#include "../plus_mode_cost/independent_plus_tone_cost.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Exact original cost graph extended to0..15 tones per band. Uses the unchanged
 * plus_tone_mode_state layout. Count-Huffman mode1 costs0x4000 per band at count
 * >=8, as proven by original3ea160; direct4-bit coding remains available. */
int32_t plus_tone_prepare_selected_v2(plus_tone_mode_state*,uint32_t);
int32_t plus_tone_prepare_stereo_map_v2(plus_tone_mode_state*,uint32_t);
int32_t plus_tone_mode_candidate_v2(plus_tone_mode_state*,uint32_t,plus_tone_substage,uint32_t);
int32_t plus_tone_choose_component_v2(plus_tone_mode_state*,uint32_t,plus_tone_substage);
int32_t plus_tone_choose_channel_v2(plus_tone_mode_state*,uint32_t,int32_t);
int32_t plus_tone_choose_modes_v2(plus_tone_mode_state*,int32_t);
#ifdef __cplusplus
}
#endif
#endif
