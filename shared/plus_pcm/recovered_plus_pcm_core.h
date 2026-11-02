#ifndef SONY_RECOVERED_PLUS_PCM_CORE_H
#define SONY_RECOVERED_PLUS_PCM_CORE_H
#include "../plus_analysis/independent_plus_pcm.h"
#include "../plus_time_features/recovered_plus_time_features.h"
#include "../plus_spectrum_encoder/recovered_plus_spectrum_encoder.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Complete default PCM composition. See separate whole-stream reports for
 * finite verification scope. The state owns mathematical history and scratch
 * arrays, never Sony code or pointers. */
typedef struct sony_plus_pcm_core_state {
    sony_plus_pcm_config pcm_config;
    sony_plus_spectrum_config spectrum_config;
    sony_plus_pcm_buffers buffers;
    sony_plus_analysis_channel channel[2];
    sony_plus_time_frame feature[3];
    uint32_t feature_order[3],evolving_flags,codec_status;
    int32_t error,failed,channel_sources[16];
    sony_plus_pcm_frame_info frame_info;
    sony_plus_spectrum_state spectrum;
    float planar_input[2][2048];
    sony_plus_spectrum_input spectrum_input;
} sony_plus_pcm_core_state;
/* arrangement is the factory unit arrangement and allocator request_mode,
 * distinct from the common preprocessing mode2. Factory preset selection is
 * verified by the separate startup and spectrum integrations. */
int sony_recovered_plus_pcm_core_initialize(uint32_t bitrate,int32_t arrangement,
    sony_plus_pcm_core_state *state);
/* One complete analysis call, including calls before the first packed frame.
 * Supply a full2048 signed16 L/R-pair block, padded with zeros by the caller.
 * Returns body bits, negative on error. All history advances exactly once.
 * After failure reinitialize the context before further input. */
int32_t sony_recovered_plus_pcm_core_analyze(sony_plus_pcm_core_state*,
    const int16_t pcm[4096]);
/* Physical frame packing is separate from the original startup/FIFO policy.
 * Calls1/2 are actual stream analysis, not extra calibration calls. */
int32_t sony_recovered_plus_pcm_core_pack(sony_plus_pcm_core_state*,
    uint8_t *output,uint32_t capacity);
#ifdef __cplusplus
}
#endif
#endif
