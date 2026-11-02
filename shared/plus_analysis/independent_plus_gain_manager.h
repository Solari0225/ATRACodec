#ifndef SONY_INDEPENDENT_PLUS_GAIN_MANAGER_H
#define SONY_INDEPENDENT_PLUS_GAIN_MANAGER_H
#include "independent_plus_gain_cross.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_analysis_channel {
    float subband[16][324];
    float neutral_overlap[16][64];
    float spectrum[2048],neutral_spectrum[2048];
    sony_plus_gain_band_history gain_history[16];
    sony_plus_gain_record previous_gain[16],current_gain[16];
    uint32_t window_flags[16];
    int32_t prior_gain_count[16];
} sony_plus_analysis_channel;
typedef struct sony_plus_gain_manager_config {
    /* max_qu is the legacy name of the native profileID at settings+1e8,
     * e.g64kbps=15,352kbps=30. It is NOT settings+b4quant-unit count. */
    int32_t mode,channels,arrangement,max_qu,bands,low_band_limit;
    uint32_t evolving_flags;
    int32_t force_gain_off,policy_force,coherence_start;
    int32_t channel_source[16];
} sony_plus_gain_manager_config;
/* Complete default mode2 orchestration of403bb0. Inputs and rolling state are
 * typed mathematical arrays. Supports mono/stereo and default policy_force0.
 * Alternate native policies/modes are explicitly rejected until recovered.
 * A caller rotates frame history before invoking this function, as3dbfd0does.
 * The neutral spectrum updates the first2bands, preserving its other bands. */
int sony_recovered_plus_gain_manager(const sony_plus_gain_manager_config *config,
    sony_plus_analysis_channel channel[2],const float contrast[16],
    const float secondary_contrast[16],int32_t coherence[16],int32_t *error);
#ifdef __cplusplus
}
#endif
#endif
