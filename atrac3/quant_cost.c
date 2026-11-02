#include "quant_cost.h"
#include <float.h>
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 &&
               FLT_MAX_EXP == 128, "IEEE binary32 required");
#include "quant_cost_tables.inc"

static float word_to_float(uint32_t value)
{
    union { uint32_t word; float number; } data;
    data.word = value;
    return data.number;
}

static uint32_t rounded_word(float input, float scale)
{
    union { uint32_t word; float number; } data;
    /* Two distinct binary32 operations, preserving the observed MULPS/ADDPS.
     * The magic add performs the original integer rounding in mantissa bits.
     */
    volatile float product = input * scale;
    volatile float shifted = product + word_to_float(UINT32_C(0x4b400000));
    data.number = shifted;
    return data.word;
}

int32_t sony_recovered_atrac3_quant_cost(uint32_t word_length,
                                        uint32_t scale_factor,
                                        uint32_t count,
                                        const float *samples)
{
    uint32_t bits = 6;
    if (!samples || word_length < 1 || word_length > 7 || scale_factor > 63 ||
        count < 8 || count > 1024 || (count != 8 && (count & 15))) return -1;

    /* The original uses (43*sf)>>7 for exact sf/3 over sf=0..63, then forms
     * the exponent adjustment by integer subtraction in IEEE encoding.
     */
    const uint32_t index = 3 * word_length - scale_factor % 3;
    const float scale = word_to_float(scale_words[index] -
                                      ((scale_factor / 3) << 23));
    if (word_length == 1) {
        for (uint32_t i = 0; i < count; i += 4) {
            uint32_t nonzero = 0;
            for (uint32_t lane = 0; lane < 4; ++lane)
                if (rounded_word(samples[i + lane],scale) != quant_masks[1])
                    nonzero |= UINT32_C(1) << lane;
            bits += nonzero_lengths[nonzero];
        }
    } else {
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t index_word = rounded_word(samples[i],scale) &
                                         quant_masks[word_length];
            bits += vlc_lengths[word_length][index_word];
        }
    }
    return (int32_t)bits;
}
