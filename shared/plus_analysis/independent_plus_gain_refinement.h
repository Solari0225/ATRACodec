#ifndef SONY_INDEPENDENT_PLUS_GAIN_REFINEMENT_H
#define SONY_INDEPENDENT_PLUS_GAIN_REFINEMENT_H
#include "independent_plus_gain_segments.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Exact24byte numeric candidate used by original refinement helpers. */
typedef struct sony_plus_gain_candidate {
    float score;
    int32_t start,end,priority,exponent_delta;
    float bound;
} sony_plus_gain_candidate;
/* Complete409e00: normalize to terminal exponent then clamp-6..9and count
 * transitions. The terminal exponent participates in the subtraction. */
int32_t sony_recovered_plus_normalize_gain_levels(int32_t levels[33]);
/* Complete409cb0: remove ascending(score+priority)candidates until at most7
 * transitions remain; preserve native difference edits and removed bound65536. */
int32_t sony_recovered_plus_prune_gain_candidates(int32_t levels[33],
    sony_plus_gain_candidate *candidates,uint32_t candidate_count,int32_t differences[32]);
/* Complete40a3f0: lower the normal baseline using applicable tail candidates. */
int sony_recovered_plus_refine_gain_baseline(sony_plus_gain_baseline *record,
    const sony_plus_gain_candidate *candidates,uint32_t candidate_count,int32_t minimum_index);
/* Complete40a450: maximum of32envelopes weighted by the supplied exact2^gain,
 * retaining original binary64 comparison and binary32 state stores. */
int sony_recovered_plus_weighted_gain_maximum(const float envelope[32],
    const int32_t levels[32],float *maximum);
#ifdef __cplusplus
}
#endif
#endif
