#ifndef SONY_RECOVERED_ATRAC3_ALLOCATION_HELPERS_H
#define SONY_RECOVERED_ATRAC3_ALLOCATION_HELPERS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* +0x463f90: arithmetic-right-shift metric, then lower bound and cap at7.
 * The x86 shift count is masked to5bits; signed inputs are intentional. */
int32_t sony_recovered_atrac3_clamp_word_length(int32_t metric,
                                              uint32_t shift,int32_t minimum);
/* +0x463e80: choose number of256-coefficient bands from the last QU boundary
 * and return gain-point header bits (3perband plus9pergainpoint).
 * QU must be1..32, gain_counts must contain4 values in0..7. */
int32_t sony_recovered_atrac3_gain_header_bits(uint32_t quantization_units,
                                const uint32_t gain_counts[4],uint32_t *bands);
/* +0x463ee0: remove unused high bands, leaving at least1, and return the
 * exact syntax-bit refund. tonal_band_flags is group-major [4][4].
 * Valid domains: bands1..4, group_count0..4; all flags boolean.
 * The actual extended-header exception is preserved. */
int32_t sony_recovered_atrac3_trim_bands(uint32_t bands,
    uint32_t extended_header,uint32_t header_field,uint32_t tonal_group_count,
    const uint32_t gain_counts[4],const uint32_t tonal_band_flags[4][4],
    uint32_t *new_bands);
/* +0x463e10: original coarse distortion/bit-demand estimator, not the exact
 * VLC cost callback. Valid unit0..31, WL1..7, SF0..63, estimates0..63. */
int32_t sony_recovered_atrac3_heuristic_cost(uint32_t unit,
    const uint32_t estimates[256],uint32_t word_length,uint32_t scale_factor);
#ifdef __cplusplus
}
#endif
#endif
