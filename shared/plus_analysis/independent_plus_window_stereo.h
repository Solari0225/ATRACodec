#ifndef SONY_INDEPENDENT_PLUS_WINDOW_STEREO_H
#define SONY_INDEPENDENT_PLUS_WINDOW_STEREO_H
#include "independent_plus_window.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original40a8f0stereo window coherence. The two output flag arrays are kept
 * separate from the record flag because the native function copies each field
 * independently. Usually the enclosing selector has already made them equal. */
int sony_recovered_plus_unify_windows(sony_plus_window_result result[2][16],
    uint32_t output_flags[2][16],const float contrast[16],uint32_t coherence_start,
    const int32_t channel_source_flags[16],uint32_t bands);
#ifdef __cplusplus
}
#endif
#endif
