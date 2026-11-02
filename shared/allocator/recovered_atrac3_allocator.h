#ifndef SONY_RECOVERED_ATRAC3_ALLOCATOR_H
#define SONY_RECOVERED_ATRAC3_ALLOCATOR_H
#include "../tonal_analysis/recovered_atrac3_tonal_analysis.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_atrac3_allocation_config {
    uint32_t maximum_quantization_units;
    int32_t bit_budget;
    uint32_t extended_header,header_field;
    /* Original analysis word2d58: previous low-band gain-point count, copied
     * before committing this frame's count by the recovered465070 parent. */
    int32_t refinement_trigger;
    uint32_t gain_count[4];
} sony_atrac3_allocation_config;
/* Original463390 full tonal/WL/SF allocation and refinement stage, given
 * transformed input spectrum and upstream analysis/gain/header parameters.
 * state->spectrum contains input MDCT values on entry and residual on exit.
 * Other state fields/components are retained where the original retains them.
 * This is not PCM analysis, gain selection, a profile factory or a full encoder.
 * Return original consumed-bit estimate, negative on invalid state.
 */
int32_t sony_recovered_atrac3_allocate(const sony_atrac3_allocation_config *config,
                                      sony_atrac3_tonal_analysis_state *state);
#ifdef __cplusplus
}
#endif
#endif
