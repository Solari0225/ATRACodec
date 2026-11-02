#ifndef SONY_INDEPENDENT_PLUS_GAIN_INTERVALS_H
#define SONY_INDEPENDENT_PLUS_GAIN_INTERVALS_H
#include "independent_plus_gain_refinement.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_gain_interval {int32_t start,end;float maximum;} sony_plus_gain_interval;
typedef struct sony_plus_gain_interval_queue {
    int32_t count,start[32],end[32];
    float maximum[32];
    int32_t classification[32];
} sony_plus_gain_interval_queue;
/* Only fields actually used by the refinement coordinator and its children. */
typedef struct sony_plus_gain_refinement_record {
    float maximum,upper_maximum,low_band_scale,weighted_maximum,selected_baseline;
    uint32_t baseline_state;
} sony_plus_gain_refinement_record;
typedef struct sony_plus_gain_candidate_state {
    int32_t levels[33],differences[32];
    sony_plus_gain_candidate candidate[32];
    uint32_t candidate_count;
    int32_t transitions;
} sony_plus_gain_candidate_state;
/* Whole409a30interval candidate insertion. Envelope has65entries because
 * tail intervals may extend to64and inspect their right-hand boundary. */
int sony_recovered_plus_insert_gain_interval(const sony_plus_gain_segment *segment,
    const sony_plus_gain_interval *interval,sony_plus_gain_interval_queue *queue,
    const float envelope[65],const sony_plus_gain_refinement_record *record,
    sony_plus_gain_candidate_state *state,float reference_maximum);
/* Whole409e70pending-interval refinement, composing the actual insertion logic. */
int sony_recovered_plus_refine_gain_intervals(const sony_plus_gain_refinement_record *record,
    sony_plus_gain_candidate_state *state,const sony_plus_gain_interval_queue *queue,
    const float envelope[65],float reference_maximum);
#ifdef __cplusplus
}
#endif
#endif
