#ifndef SONY_RECOVERED_PLUS_BUDGET_H
#define SONY_RECOVERED_PLUS_BUDGET_H
#include <stdint.h>
#include "../plus_sf_wl/recovered_plus_words.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_priority_channel {int32_t coarse[32];float peakiness[32];} sony_plus_priority_channel;
/* Complete original3f12b0: descending Shell sort, in-place score writes and
 * index permutations, including original equal-score behavior across gaps. */
int sony_recovered_plus_priority_sort(int32_t *scores,int32_t *indices,uint32_t count);
/* Original3e7c00 priority construction, immediately followed by real sort.
 * Scores/indices contain channels*units adjacent entries, not stride32. */
int sony_recovered_plus_priorities(uint32_t channels,uint32_t units,int32_t special_mode,
    const sony_plus_priority_channel input[2],int32_t *scores,int32_t *indices);
/* Complete original3ed700: trial quantization and best short-book selection.
 * Copies no existing rows: caller supplies original candidate matrix storage.
 * Return0/-1; signed16 difference is separate. Threshold is the feature's
 * dead-zone index, while current_book is codec+1b578's short-table selector. */
int sony_recovered_plus_trial_cost(uint32_t unit,uint32_t word_length,
    int32_t current_book,const sony_plus_unit_cost_input *input,
    const sony_plus_cost_matrix *current,sony_plus_cost_matrix *candidate,
    int32_t *selected_book,int32_t *difference);
#ifdef __cplusplus
}
#endif
#endif
