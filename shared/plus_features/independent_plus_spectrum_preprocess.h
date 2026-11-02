#ifndef INDEPENDENT_PLUS_SPECTRUM_PREPROCESS_H
#define INDEPENDENT_PLUS_SPECTRUM_PREPROCESS_H
#include "independent_plus_spectrum_stats.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct plus_spectrum_prep_config {
    uint32_t channels,units,subbands,backend;
    int32_t profile_id,sample_rate,request_mode,force_gain_off;
    uint32_t feature_flags,coherence_start,source_swap[16];
} plus_spectrum_prep_config;
typedef struct plus_spectrum_prep_channel {
    float reference_spectrum[2048];
    plus_given_gain_channel gain;
    plus_given_gain_band previous_gain[16];
    int32_t neutral_coarse[32],previous_neutral_coarse[32];
    float current_peakiness[32],previous_peakiness[32];
    uint32_t word_length[32],book[32];
    int16_t maximum_word_length,unit_limits[32],boundary,activity;
    int32_t significant[32];
    float reference_peak[32],average_peakiness[32],statistics[2];
    float demand_scale,average_tonality;
    uint32_t gain_flag_word;
} plus_spectrum_prep_channel;
typedef struct plus_spectrum_prep_shared {
    /* Native cfg+90,+94,+98,+9c,+a0,+118. */
    uint32_t header[6];
    /* Native shared+84,+88,+8c,+90. */
    uint32_t words[4];
    int16_t stereo[32],trailing[32];
} plus_spectrum_prep_shared;
/* Complete original3e02b0 spectrum/feature preparation. The main gain-compensated
 * spectrum and coarse/numerator output use a contiguous plus_feature_channel[2]
 * so the existing complete3e1030 API can consume them directly, in place.
 * Reference spectra and current/prior gain records come from the independent
 * preceding PCM analysis stage, not from a parsed original core.
 * Persistent neutral_coarse/current_peakiness arrays rotate with the caller's
 * gain records; previous arrays are immutable inputs. Units0..32, subbands0..16,
 * profile0..31, channels1/2, coherence_start0..16. Distinct main/reference arrays.
 * Active samples and previous metrics are finite; gain points<=7, levels0..15.
 * Returns0/-1. No supplied allocation or bitstream decisions are required. */
int plus_spectrum_preprocess(const plus_spectrum_prep_config*,
    plus_feature_channel normalized[2],plus_spectrum_prep_channel channels[2],
    plus_spectrum_prep_shared*);
#ifdef __cplusplus
}
#endif
#endif
