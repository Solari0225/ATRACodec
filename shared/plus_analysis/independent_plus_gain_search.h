#ifndef SONY_INDEPENDENT_PLUS_GAIN_SEARCH_H
#define SONY_INDEPENDENT_PLUS_GAIN_SEARCH_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_gain_search_points {
    int32_t count,location[7],increment[7];
    float fraction[7];
} sony_plus_gain_search_points;
typedef struct sony_plus_gain_forward_request {
    int32_t length,step,point_count,opposite_count,difference_mode;
    float floor,secondary_floor,threshold;
} sony_plus_gain_forward_request;
typedef struct sony_plus_gain_forward_state {
    int32_t truncate_first,total_delta,limit,used_delta;
} sony_plus_gain_forward_state;
typedef struct sony_plus_gain_backward_request {
    int32_t stop,step,opposite_count,point_count,freeze_lookahead,difference_mode;
    float floor,threshold;
} sony_plus_gain_backward_request;
typedef struct sony_plus_gain_backward_state {
    int32_t used_delta,limit;
    float first_peak;
} sony_plus_gain_backward_state;
/* Whole original408f50/409130candidate scans. The requests carry native value
 * arguments, states carry native pointer arguments. point.count is retained
 * unchanged; as in the native callers, use the returned count to commit it. */
int32_t sony_recovered_plus_search_gain_forward(const sony_plus_gain_forward_request *request,
    sony_plus_gain_forward_state *state,const float envelope[33],sony_plus_gain_search_points *points);
int32_t sony_recovered_plus_search_gain_backward(const sony_plus_gain_backward_request *request,
    sony_plus_gain_backward_state *state,const float envelope[33],const float paired_envelope[33],
    sony_plus_gain_search_points *points);
#ifdef __cplusplus
}
#endif
#endif
