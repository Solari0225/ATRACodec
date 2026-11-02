#ifndef INDEPENDENT_PLUS_QUANT_FIELDS_H
#define INDEPENDENT_PLUS_QUANT_FIELDS_H
#include "../plus_packer/independent_plus_packer.h"
#ifdef __cplusplus
extern "C" {
#endif
/* All parameters and prepared vectors are already chosen upstream. This module
 * packs them; it does not derive word lengths, scale factors, weights or modes.
 * Arithmetic on signed prepared values uses their uint32 two's-complement bits.
 */
typedef struct plus_given_word_length {
    uint32_t channel_index,mode;
    uint32_t prefix_count,suffix_width,suffix_base,weight_id,huffman_id;
    uint32_t range_mode,packed_count,fill_extra,mode2_param4,mode2_param3,group_enable;
    uint32_t raw[32],values[32],residual[32];
} plus_given_word_length;
typedef struct plus_given_word_length_global {
    int32_t total_units,group_count;
    uint32_t group_flags[16];
} plus_given_word_length_global;
int32_t plus_pack_word_length_values(plus_bit_writer*,const plus_given_word_length_global*,
    const plus_given_word_length*,const plus_given_word_length* reference);
int32_t plus_pack_word_lengths(plus_bit_writer*,const plus_given_word_length_global*,
    const plus_given_word_length* channels,int32_t channel_count);

typedef struct plus_given_scale_factor {
    uint32_t channel_index,mode;
    uint32_t prefix_count,suffix_width,suffix_base,huffman_id,weight_id,offset_b,offset_a;
    uint32_t raw[32],prepared[4][32];
} plus_given_scale_factor;
int32_t plus_pack_scale_factor_values(plus_bit_writer*,int32_t active_units,
    const plus_given_scale_factor*,const plus_given_scale_factor* reference);
int32_t plus_pack_scale_factors(plus_bit_writer*,int32_t active_units,
    const plus_given_scale_factor* channels,int32_t channel_count);
/* Exact stored symbol widths for a caller's independent cost calculation.
 * A width>32 (including0xff) is outside the bounded writer's encodable domain;
 * it is retained as observed numerical data, not silently replaced by a code.
 */
int32_t plus_word_length_symbol_width(uint32_t table,uint32_t symbol);
int32_t plus_scale_factor_symbol_width(uint32_t short_alphabet,uint32_t table,uint32_t symbol);
#ifdef __cplusplus
}
#endif
#endif
