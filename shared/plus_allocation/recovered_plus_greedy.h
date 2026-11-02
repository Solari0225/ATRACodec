#ifndef SONY_RECOVERED_PLUS_GREEDY_H
#define SONY_RECOVERED_PLUS_GREEDY_H
#include "recovered_plus_budget.h"
#include "../plus_codebook_cost/independent_plus_fast_wl.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_greedy_config {uint32_t channels,units,word_units;int32_t special_mode;} sony_plus_greedy_config;
typedef struct sony_plus_greedy_channel {
    sony_plus_unit_cost_input input;
    int32_t maximum_word_length,word_length[32],selected_book[32];
    sony_plus_priority_channel priority;
    plus_fast_wl_parameters modes;
} sony_plus_greedy_channel;
typedef struct sony_plus_greedy_shared {
    int32_t reserve,enabled;int16_t word_bits,overhead,total,channel_cost[2];
    sony_plus_cost_matrix matrix[2];
} sony_plus_greedy_shared;
/* Complete original3e7c00 greedy refinement: original priority sort, seven
 * passes, twenty trial limit, quantization, incremental WL modes and rollback.
 * The four live cost candidates per QU are preserved. Original matrix rows
 * also contain four unused short slots copied from uninitialized stack in
 * accepted trials; this independent representation retains their input value.
 * They are never consumed by the recovered encoder's cost/packing algorithms. */
int sony_recovered_plus_greedy_words(const sony_plus_greedy_config *config,
    sony_plus_greedy_channel channels[2],sony_plus_greedy_shared *shared,int32_t budget);
#ifdef __cplusplus
}
#endif
#endif
