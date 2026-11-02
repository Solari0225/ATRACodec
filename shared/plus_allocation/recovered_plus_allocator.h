#ifndef SONY_RECOVERED_PLUS_ALLOCATOR_H
#define SONY_RECOVERED_PLUS_ALLOCATOR_H
#include "recovered_plus_initial.h"
#include "recovered_plus_greedy.h"
#include "recovered_plus_refine.h"
#include "../plus_sf_wl/recovered_plus_noise_region.h"
#include "../plus_mode_cost/independent_plus_gain_cost.h"
#include "../plus_tone_cost_v2/independent_plus_tone_cost_v2.h"
#include "../plus_fields/independent_plus_quant_fields.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_allocator_config {
    sony_plus_initial_config initial;
    uint32_t scale_groups,full_table,simd,noise_present;
} sony_plus_allocator_config;
typedef struct sony_plus_allocator_flags {uint32_t present,explicit_values,values[16];} sony_plus_allocator_flags;
typedef struct sony_plus_allocator_channel {
    sony_plus_initial_channel analysis;
    sony_plus_unit_cost_input input;
    sony_plus_sf_choice_channel sf;
    plus_fast_wl_parameters modes;
    plus_given_gain_channel gain;
    plus_given_gain_location_parameters gain_location;
    sony_plus_gain_points previous_gain[16];
    sony_plus_allocator_flags window;
    plus_given_word_length words;
    int32_t selected_book[32],method_fill_extra[3];
    int16_t quantized[2048];int32_t noise_region[5];
} sony_plus_allocator_channel;
typedef struct sony_plus_allocator_state {
    sony_plus_allocator_channel channel[2];
    sony_plus_initial_stereo stereo;
    sony_plus_initial_result limits;
    plus_tone_mode_state tones;
    uint32_t stereo_modes[2][2],stereo_second[16];
    sony_plus_cost_matrix matrix[2];
    int16_t costs[14],threshold_limit,tail_unit;
    int32_t word_modes_enabled,scale_modes_enabled,reserve,actual_bits,budget;
} sony_plus_allocator_state;
/* Full original3e6330 initialization and header decisions. Upstream analysis
 * must already supply normalized spectra, gains, tones and typed metrics.
 * stereo.spectrum is the shared original normalized quantization input;
 * channel.input.normalized is synchronized at stage boundaries. */
int sony_recovered_plus_allocator_initialize(const sony_plus_allocator_config*,
    sony_plus_allocator_state*,int32_t budget);
/* Complete default original3e5be0 allocation coordinator. Returns actual body
 * bits, or -1 for invalid input. Does not perform PCM analysis or pack a frame.
 * The uninitialized spare matrix columns retain their original input values,
 * as documented by the separately verified greedy algorithm. */
int32_t sony_recovered_plus_allocate(const sony_plus_allocator_config*,
    sony_plus_allocator_state*,int32_t budget);
#ifdef __cplusplus
}
#endif
#endif
