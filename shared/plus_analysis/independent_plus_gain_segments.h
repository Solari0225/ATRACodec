#ifndef SONY_INDEPENDENT_PLUS_GAIN_SEGMENTS_H
#define SONY_INDEPENDENT_PLUS_GAIN_SEGMENTS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_gain_segment {
    int32_t start,end,previous_direction,next_direction,classification;
    float maximum,baseline;
} sony_plus_gain_segment;
typedef struct sony_plus_gain_baseline {
    float lower,upper,low_band_scale,selected;
    uint32_t state;
} sony_plus_gain_baseline;
/* Complete409830: split32envelopes by33gain exponents. Returns the number of
 * transitions, with transition_count+1segments. Unwritten difference entries,
 * classification and baseline fields retain their input values. */
int32_t sony_recovered_plus_gain_segments(const float envelope[32],
    const int32_t levels[33],sony_plus_gain_segment segments[33],int32_t differences[32]);
/* Complete409920: classify each segment by entering/leaving direction. */
int sony_recovered_plus_classify_gain_segments(const float envelope[33],
    sony_plus_gain_segment *segments,uint32_t segment_count,
    float lower_reference,float upper_reference);
/* Complete40a150 /40a2a0: choose baseline from trailing unchanged envelope.
 * The normal-domain helper clears state; the difference-domain helper preserves
 * it, because its native record has no corresponding state write. */
int32_t sony_recovered_plus_gain_baseline_normal(sony_plus_gain_baseline *record,
    const float envelope[33],const int32_t differences[32],uint32_t band,
    float fallback,int32_t mode);
int32_t sony_recovered_plus_gain_baseline_difference(sony_plus_gain_baseline *record,
    const float envelope[33],const int32_t differences[32],uint32_t band,
    float fallback,int32_t mode);
#ifdef __cplusplus
}
#endif
#endif
