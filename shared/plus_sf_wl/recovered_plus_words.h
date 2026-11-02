#ifndef SONY_RECOVERED_PLUS_WORDS_H
#define SONY_RECOVERED_PLUS_WORDS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_cost_matrix {int32_t book[32];int16_t costs[32][8];} sony_plus_cost_matrix;
typedef struct sony_plus_unit_cost_input {
    uint32_t bank,simd;int32_t threshold_index[32];float threshold_factor[32],normalized[2048];
} sony_plus_unit_cost_input;
typedef struct sony_plus_word_channel {
    sony_plus_unit_cost_input input;
    int32_t word_length[32],significant[32],history[32],changed[32];float demand[32];
    int16_t maximum_word_length,boundary_word;
} sony_plus_word_channel;
typedef struct sony_plus_word_config {uint32_t channels,units,sample_rate,profile;int32_t special_mode;} sony_plus_word_config;
typedef struct sony_plus_word_shared {
    int16_t threshold_limit,tail_unit,overhead,auxiliary_total,total,channel_cost[2],stereo_flags[32];
    sony_plus_cost_matrix matrix[2];
} sony_plus_word_shared;
/* Original3ed390/3ed0f0. NULL changed means recalculate all active units;
 * otherwise only flagged units are recalculated and other rows remain cached.
 * Inactive units reset selected book0. Ties choose first short-table candidate.
 * Matrix is exactly original640 numeric bytes; only first4 of8 costs per QU
 * are touched. Return0/-1; total uses original signed16 sum semantics. */
int sony_recovered_plus_unit_costs(const sony_plus_unit_cost_input *input,
    const int32_t word_lengths[32],const int32_t *changed,uint32_t units,
    sony_plus_cost_matrix *matrix,int16_t *total);
/* Original3e7ba0 cached all-zero unit representation; preserves WL itself. */
int sony_recovered_plus_zero_unit(uint32_t bank,uint32_t unit,uint32_t word_length,
    sony_plus_cost_matrix *matrix,int32_t *saved_bits);
/* Original3e6cb0 threshold initializer,3e70b0 iterative WL adjustment,
 * 3e76e0 tail threshold increase and3e7960 spectral tail zeroing. */
int sony_recovered_plus_word_thresholds(const sony_plus_word_config *config,
    sony_plus_word_channel channels[2],sony_plus_word_shared *shared,int32_t budget);
int sony_recovered_plus_word_search(const sony_plus_word_config *config,
    sony_plus_word_channel channels[2],sony_plus_word_shared *shared,int32_t budget);
int sony_recovered_plus_threshold_trim(const sony_plus_word_config *config,
    sony_plus_word_channel channels[2],sony_plus_word_shared *shared,int32_t budget);
int sony_recovered_plus_spectrum_trim(const sony_plus_word_config *config,
    sony_plus_word_channel channels[2],sony_plus_word_shared *shared,
    float *spectra[2],int32_t budget);
#ifdef __cplusplus
}
#endif
#endif
