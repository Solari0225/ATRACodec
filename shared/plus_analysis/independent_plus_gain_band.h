#ifndef SONY_INDEPENDENT_PLUS_GAIN_BAND_H
#define SONY_INDEPENDENT_PLUS_GAIN_BAND_H
#include "independent_plus_gain_math.h"
#ifdef __cplusplus
extern "C" {
#endif
/* All38native words are typed from actual integer/FPU reads and writes.
 * The first64bytes are the separately recovered gain point structure. */
typedef struct sony_plus_gain_record {
    sony_plus_gain_points points;
    int32_t rise_sum,fall_sum;
    float maximum,upper_maximum,edge_maximum,low_band_scale;
    int32_t forward_count,first_forward_location,last_backward_location;
    float peak_ratio;
    int32_t previous_turns,current_turns;
    float weighted_maximum,selected_baseline;
    int32_t baseline_state;
    float difference_maximum,difference_upper_maximum,difference_baseline;
    int32_t difference_rise_sum,difference_fall_sum;
    float envelope_sum,difference_envelope_sum;
} sony_plus_gain_record;
typedef struct sony_plus_gain_band_history {
    float envelope[33],difference[32];
} sony_plus_gain_band_history;
typedef struct sony_plus_gain_band_request {
    /* max_qu stores profileID(settings+1e8), not quant-unit count. */
    int32_t mode,channels,max_qu,coherence_start,band,upper_band;
} sony_plus_gain_band_request;
/* Original4073e0band selection. Input is exactly native sample127..259:
 * one preceding sample,128current-lookahead samples,4following samples.
 * Neighboring current bands1..3are consulted only by band0cross-band policies.
 * error corresponds only to the native common+0xc numeric error field. */
int sony_recovered_plus_select_gain_band(const sony_plus_gain_band_request *request,
    const float input[133],sony_plus_gain_band_history *history,
    const sony_plus_gain_record previous[16],sony_plus_gain_record current[16],int32_t *error);
#ifdef __cplusplus
}
#endif
#endif
