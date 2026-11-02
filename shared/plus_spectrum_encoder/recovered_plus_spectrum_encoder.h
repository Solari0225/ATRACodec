#ifndef SONY_RECOVERED_PLUS_SPECTRUM_ENCODER_H
#define SONY_RECOVERED_PLUS_SPECTRUM_ENCODER_H
#include "../plus_features/independent_plus_spectrum_preprocess.h"
#include "../plus_allocation/recovered_plus_allocated_frame.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct sony_plus_spectrum_config {
    plus_spectrum_prep_config preprocess;
    sony_plus_allocator_config allocation;
    sony_plus_allocated_frame_parameters frame;
} sony_plus_spectrum_config;

/* Narrow upstream statistics, not allocation decisions. Native unit+0,+4,
 * +84,+104,+184,+2c4 respectively. */
typedef struct sony_plus_spectrum_statistics {
    int32_t coherence_boundary,coherence[16];
    float contrast[16],left_energy[16],right_energy[16],latest_contrast[16];
} sony_plus_spectrum_statistics;

/* Quantized upstream tone analysis, without any coding-mode decisions. */
typedef struct sony_plus_spectrum_tones {
    uint32_t present,amplitude_mode;
    int32_t band_count;
    uint32_t flags[3][16],previous_envelope[2][16][4];
    plus_given_tone_band bands[2][16];
} sony_plus_spectrum_tones;

typedef struct sony_plus_spectrum_input {
    float main_spectrum[2][2048],neutral_spectrum[2][2048];
    sony_plus_gain_points current_gain[2][16],previous_gain[2][16];
    uint32_t window_flags[2][16],source_swap[16];
    uint32_t evolving_flags;
    sony_plus_spectrum_statistics statistics;
    sony_plus_spectrum_tones tones;
} sony_plus_spectrum_input;

typedef struct sony_plus_spectrum_history {
    int32_t neutral_coarse[32];
    float peakiness[32];
} sony_plus_spectrum_history;

typedef struct sony_plus_spectrum_state {
    plus_feature_channel normalized[2];
    plus_spectrum_prep_channel preparation[2];
    plus_spectrum_prep_shared prepared_shared;
    plus_feature_shared normalized_shared;
    sony_plus_allocator_state allocated;
    sony_plus_spectrum_history history[2][2];
    uint32_t analysis_frames;
} sony_plus_spectrum_state;

/* Original nine default CD-stereo factory presets; unknown rates rejected.
 * Common mode2, backend1024, source-feature flags evolve per input frame.
 * Factory requested QU/bands remain distinct from effective preprocessing QU. */
int sony_recovered_plus_spectrum_default(uint32_t bitrate,sony_plus_spectrum_config*);
int sony_recovered_plus_spectrum_initialize(sony_plus_spectrum_state*);
/* Full raw-spectrum -> feature prep -> normalization -> independent allocator.
 * The initial two stream analysis calls also advance this state. Stream/FIFO
 * policy is owned by the separate caller, not inferred or calibrated here.
 * Returns independently computed body bits, or a negative error. */
int32_t sony_recovered_plus_spectrum_allocate(const sony_plus_spectrum_config*,
    sony_plus_spectrum_state*,const sony_plus_spectrum_input*);
/* Packs the latest independently allocated state, without changing histories. */
int32_t sony_recovered_plus_spectrum_pack(const sony_plus_spectrum_config*,
    const sony_plus_spectrum_state*,uint8_t*,uint32_t,uint32_t*);
/* Convenience for an analysis frame which the stream caller wants to pack. */
int32_t sony_recovered_plus_spectrum_encode(const sony_plus_spectrum_config*,
    sony_plus_spectrum_state*,const sony_plus_spectrum_input*,uint8_t*,uint32_t,uint32_t*);
#ifdef __cplusplus
}
#endif
#endif
