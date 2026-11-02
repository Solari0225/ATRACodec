#ifndef SONY_RECOVERED_ATRAC3_TONAL_ANALYSIS_H
#define SONY_RECOVERED_ATRAC3_TONAL_ANALYSIS_H
#include "../sideinfo/recovered_atrac3_sideinfo.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_atrac3_tonal_slots {
    uint32_t count;
    /* Zero means empty;1..64 refers to components[id-1]. No native pointers. */
    uint32_t component_ids[7];
} sony_atrac3_tonal_slots;
typedef struct sony_atrac3_tonal_analysis_group {
    uint32_t band_present[4],word_length,coefficients_minus_one;
    sony_atrac3_tonal_slots window[16];
} sony_atrac3_tonal_analysis_group;
typedef struct sony_atrac3_tonal_analysis_state {
    float spectrum[1024];
    uint32_t word_length[32],scale_factor[32];
    uint32_t quantization_units,subbands,tonal_mode,tonal_group_count;
    sony_atrac3_tonal_analysis_group group[2];
    sony_atrac3_tonal_component components[64];
} sony_atrac3_tonal_analysis_state;
/* Original464010 high-budget tonal analysis branch. It chooses candidate
 * locations/groups, quantizes tones, handles window saturation/replacement,
 * updates residual spectrum, estimates and initial residual WL/SF metrics.
 * It does not choose which allocation branch to call or implement gain
 * analysis or the final WL/SF refinement loop. The alternate branch is below.
 *
 * Valid input: available_bits>=0, QU1..32, subbands1..4, all component initial
 * SF fields0..63, finite audio-scale spectra. metrics[32]/estimates[256] are
 * in/out; inactive entries and unused component words are preserved.
 * Return original syntax cost, or negative on invalid input/internal error.
 * Output must be discarded on error. Buffers must not overlap. */
int32_t sony_recovered_atrac3_analyze_tonal_greedy(int32_t available_bits,
    uint32_t metrics[32],uint32_t estimates[256],
    sony_atrac3_tonal_analysis_state *state);
/* Original4646b0 alternate branch, including overlap decisions, candidate
 * threshold adaptation, symbol shifting/splitting, coefficient-count trimming,
 * and mode0/1 cost comparison. metrics is read-only. Input residual WL0..7,
 * SF0..63 and component initial SF0..63; QU must exceed8 when budget<1100
 * (the actual branch begins at unit8 in that case). Negative SF words and
 * temporary coefficient counts in the output are original allocation markers,
 * not final encoded parameters. This does not run final allocation/refinement.
 */
int32_t sony_recovered_atrac3_analyze_tonal_alternate(int32_t available_bits,
    const uint32_t metrics[32],sony_atrac3_tonal_analysis_state *state);
#ifdef __cplusplus
}
#endif
#endif
