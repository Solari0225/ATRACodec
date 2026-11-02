#ifndef INDEPENDENT_PLUS_FRAME_H
#define INDEPENDENT_PLUS_FRAME_H
#include "../plus_fields/independent_plus_quant_fields.h"
#include "../plus_fields/independent_plus_code_tables.h"
#include "../plus_fields/independent_plus_spectrum.h"
#include "../plus_fields/independent_plus_gain.h"
#include "../plus_fields/independent_plus_tones.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original packing order with all nine real stages. Inputs are the results
 * of analysis/selection; this API does not perform PCM analysis or allocation.
 * Pointers are borrowed only for the duration of the call. */
typedef struct plus_given_frame_fields {
    uint32_t unit_id,channel_count,quantization_units,header_flag;
    uint32_t expected_body_bits,target_total_bits;
    const plus_given_word_length_global* word_global;
    const plus_given_word_length* word_channels;
    int32_t active_units;
    const plus_given_scale_factor* scale_channels;
    uint32_t full_code_table;
    const plus_given_code_table_channel* code_channels;
    const plus_given_spectrum_global* spectrum_global;
    const plus_given_spectrum_channel* spectrum_channels;
    int32_t window_subbands;
    const plus_given_flag_vector* window_channels;
    const plus_given_gain_channel* gain_channels;
    const plus_given_gain_location_parameters* gain_locations;
    const plus_given_tone_global* tone_global;
    const plus_given_tone_channel* tone_channels;
    uint32_t noise_present,noise_level,noise_table;
} plus_given_frame_fields;
int32_t plus_pack_given_channel_unit(plus_bit_writer*,const plus_given_frame_fields*,uint32_t* codec_status);
/* One OMA ATRAC3plus frame: leading zero bit, channel unit, terminator, byte
 * alignment and original0x01 fill. target_total_bits is an absolute budget. */
int32_t plus_pack_given_frame(plus_bit_writer*,const plus_given_frame_fields*,uint32_t* codec_status);
#ifdef __cplusplus
}
#endif
#endif
