#ifndef INDEPENDENT_PLUS_SPECTRUM_H
#define INDEPENDENT_PLUS_SPECTRUM_H
#include "independent_plus_flags.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Descriptor index = 7 * (selected_codebook + 8 * table_bank) + word_length.
 * Index zero is unused. All descriptors and Huffman rows are numerical data,
 * not pointers or executable code from the original implementation. */
typedef struct plus_spectrum_descriptor {
    uint32_t first_row,row_count;
    uint8_t coefficient_group,symbol_group,coefficient_shift,extra_signs;
    uint8_t coefficient_bits,coefficient_mask;
} plus_spectrum_descriptor;
const plus_spectrum_descriptor* plus_spectrum_book(uint32_t descriptor_index);
int32_t plus_spectrum_symbol_width(uint32_t descriptor_index,uint32_t symbol);
/* Pure coefficient -> symbol/sign transform. Inputs are already quantized.
 * Only N/group output entries are written. Sign arrays stay untouched when
 * extra_signs==0, matching the original mathematical helper. */
int32_t plus_spectrum_make_symbols(const int16_t* input,uint32_t n,
    uint32_t descriptor_index,int16_t* symbols,uint32_t* sign_counts,uint32_t* sign_values);
int32_t plus_pack_spectrum_unit(plus_bit_writer*,const int16_t* input,
    uint32_t n,uint32_t descriptor_index);
typedef struct plus_given_spectrum_channel {
    uint32_t table_bank;
    uint32_t codebooks[32],word_lengths[32];
    int16_t coefficients[2048];
    uint32_t trailing[16];
} plus_given_spectrum_channel;
typedef struct plus_given_spectrum_global {
    int32_t active_units,subbands;
    uint32_t full_table;
    plus_given_flag_vector stereo_flags[2];
} plus_given_spectrum_global;
int32_t plus_pack_spectrum(plus_bit_writer*,const plus_given_spectrum_global*,
    const plus_given_spectrum_channel* channels,int32_t channel_count);
#ifdef __cplusplus
}
#endif
#endif
