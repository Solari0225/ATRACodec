#ifndef SONY_INDEPENDENT_PLUS_GAIN_PROFILE_H
#define SONY_INDEPENDENT_PLUS_GAIN_PROFILE_H
#include "independent_plus_gain_intervals.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_gain_profile_request {
    uint32_t band;
    float lower_reference,upper_reference,fallback;
    int32_t difference_mode,search_count,head_state,allow_head;
} sony_plus_gain_profile_request;
/* Whole original4093d0gain-profile refinement coordinator, composing the
 * recovered baseline/segment/interval/pruning/weighted-peak procedures.
 * Only the three selected-baseline/state/weighted fields of record mutate. */
int sony_recovered_plus_refine_gain_profile(sony_plus_gain_refinement_record *record,
    const float envelope[65],int32_t levels[33],const sony_plus_gain_profile_request *request);
#ifdef __cplusplus
}
#endif
#endif
