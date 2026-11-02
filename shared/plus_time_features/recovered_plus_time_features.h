#ifndef SONY_PLUS_TIME_FEATURES_H
#define SONY_PLUS_TIME_FEATURES_H
#include "recovered_plus_time_math.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Fixed-width normalized state: no pointers, codec offsets or host ABI padding.
 * All channels share one common note pool. first_note indexes that pool.
 * envelope[] retains the eight original numerical fields in their observed
 * order; this analysis clears/uses the same fields as the original graph. */
typedef struct sony_plus_time_frame {
    sony_plus_time_common common;
    sony_plus_time_band bands[2][16];
} sony_plus_time_frame;
typedef struct sony_plus_time_feature_config {
    int32_t profile_id,bands,channels,backend;
} sony_plus_time_feature_config;
/* Initialize each of the caller's rotating feature slots, once at factory.
 * The caller selects distinct previous/current slots before process(). */
int sony_plus_time_features_initialize(sony_plus_time_frame *frame);
/* Complete independently recovered graph, accepted by the module reports.
 * Original default codec mode2 only. input[ch*16+band] addresses the middle
 * time sample192 of the full448-sample band buffer. Exactly256 readable
 * samples are required; only the first128 are modified in place.
 * previous is read-only; current is in/out and may contain older slot state.
 * No slot rotation, gain rotation or PCM filtering is performed here. Native
 * detector3f57b0 reads and may OR bit0 into evolving_flags after begin() has
 * shifted them. This is not equivalent to OR(current.common.present).
 * After the call, caller can read prior counts from previous bands. Frame
 * serialization consumes the caller's older slot, not this current slot.
 * The caller's error is preserved
 * except for the original >48 total tones condition, which writes0x8f.
 * backend0 selects original x87-style53bit arithmetic; nonzero selects its
 * binary32 SIMD graph, implemented portably without SIMD requirements. */
int sony_plus_time_features_process(const sony_plus_time_feature_config *config,
    const sony_plus_time_frame *previous,sony_plus_time_frame *current,
    float *const input[32],uint32_t *evolving_flags,int32_t *error);
#ifdef __cplusplus
}
#endif
#endif
