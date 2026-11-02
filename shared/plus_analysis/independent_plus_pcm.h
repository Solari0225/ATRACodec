#ifndef SONY_INDEPENDENT_PLUS_PCM_H
#define SONY_INDEPENDENT_PLUS_PCM_H
#include "independent_plus_gain_manager.h"
#include "independent_plus_stereo_stats.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Typed native stereo-statistics unit through+0x684. The second phase array
 * belongs to alternate modes and is preserved by this defaultmode2 API. */
typedef struct sony_plus_pcm_statistics {
    int32_t coherence_boundary,coherence[16];
    float contrast[3][16],left_energy[2][16],right_energy[2][16];
    int32_t source_flags[2][16];
    float latest_contrast[7][16],phase_coefficient[2][16];
    sony_plus_stereo_weights weights;
    float recent_left[16],recent_right[16],recent_difference[16];
} sony_plus_pcm_statistics;
typedef struct sony_plus_pcm_buffers {
    float time[2][16][448];
    float neutral_overlap[2][16][64],filter_history[2][368];
    sony_plus_pcm_statistics statistics;
} sony_plus_pcm_buffers;
typedef struct sony_plus_pcm_config {
    int32_t channels,arrangement,profile_id,sample_rate,requested_quant_units;
    int32_t force_gain_off,policy_force;
} sony_plus_pcm_config;
typedef struct sony_plus_pcm_frame_info {
    int32_t quant_units,coded_bands,quant_groups,stereo_metric_bands;
} sony_plus_pcm_frame_info;
/* Static default mathematical histories, observed before the first owned
 * official conversion and checked bytewise. No livecodec/oracle dependency. */
int sony_recovered_plus_pcm_initialize(sony_plus_pcm_buffers *buffers,
    sony_plus_analysis_channel channel[2]);
/* First3dbfd0segment: stereo-stat history, flags shift, time history shift,
 * filterbank, contrast, phase compensation, joint stereo weighting.
 * The caller rotates gain slots and time-feature slots before feature analysis.
 * Every band retains448samples.3f51e0must receive time[ch][band]+192 with
 *256consecutive readable samples and may modify those middle128samples. */
int sony_recovered_plus_pcm_begin(const sony_plus_pcm_config *config,
    const float pcm[2][2048],sony_plus_pcm_buffers *buffers,uint32_t *evolving_flags);
/* Final3dbfd0segment, AFTER real independent3f51e0 has updated buffers/time,
 * evolving flags and channel[].prior_gain_count. The two spectrum arrays in
 *channel[]are in/out so untouched spectral tails retain caller history.
 * Callers must not skip3f51e0when the selected factory enables it. */
int sony_recovered_plus_pcm_finish(const sony_plus_pcm_config *config,
    sony_plus_pcm_buffers *buffers,sony_plus_analysis_channel channel[2],
    uint32_t evolving_flags,int32_t channel_sources[16],int32_t *error,
    sony_plus_pcm_frame_info *info);
#ifdef __cplusplus
}
#endif
#endif
