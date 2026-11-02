#ifndef SONY_RECOVERED_PLUS_INITIAL_H
#define SONY_RECOVERED_PLUS_INITIAL_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_initial_config {
    uint32_t channels,maximum_units,maximum_subbands,sample_rate,profile;
    int32_t request_mode,special_mode;
} sony_plus_initial_config;
typedef struct sony_plus_initial_channel {
    int32_t maximum_word_length,activity,flag;
    int32_t significant[32],history[32];
    float demand[32],peakiness[32],scale,noise;
    uint32_t word_length[32],noise_flag;
} sony_plus_initial_channel;
typedef struct sony_plus_initial_stereo {
    float correlation[32],energy_left[16],energy_right[16];
    int32_t base[32],coarse[2][32],analyzed_bands,band_flags[16];
    float band_correlation[16],band_score[16],channel_metric[2][32];
    uint32_t context_policy;
    int32_t band_output[16];
    float spectrum[2][2048];
} sony_plus_initial_stereo;
typedef struct sony_plus_initial_result {
    uint16_t stereo_flags[32],swap_flags[32];
    uint32_t limit_units,limit_subbands,active_units,active_subbands,format_flag;
} sony_plus_initial_result;
/* Original3e6330 numeric initialization through402540 trailing-unit trim:
 * profile demand curves, activity/peakiness biases, initial WL, original
 * complete3ec440 stereo preparation, limits and noise flag. Upstream metrics
 * are caller supplied; side-info cost/mode choice and later refinements are
 * separate. Binary64 math, original binary32 stores, no FMA. Stereo preparation
 * may align coarse indices, change band flags and negate right spectra.
 */
int sony_recovered_plus_initial_words(const sony_plus_initial_config *config,
    sony_plus_initial_channel channels[2],sony_plus_initial_stereo *stereo,
    sony_plus_initial_result *result);
int sony_recovered_plus_initial_stereo_prepare(const sony_plus_initial_config*,
    const sony_plus_initial_channel channels[2],sony_plus_initial_stereo*,
    sony_plus_initial_result*);
/* Complete original3ecf20 score→WL rule, including signed16 upper bound,
 * truncation after+0.5 and clamp order. Finite bounded original metrics. */
int sony_recovered_plus_initial_round(const float *scores,int32_t *output,
    int32_t maximum,int32_t count);
#ifdef __cplusplus
}
#endif
#endif
