#ifndef ATRACODEC_PLUS_DEQUANT_FLOAT_H
#define ATRACODEC_PLUS_DEQUANT_FLOAT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Internal ordinary ATRAC3plus signed coefficient dequantization, Sony403990.
 * Word length1..7, scale factor0..63,count0..128. Requires distinct buffers.
 * Returns0, or-1 without a write for invalid arguments. */
int atracodec_plus_dequant_float(const int16_t *input,float *output,
    uint32_t word_length,uint32_t scale_factor,uint32_t count);
#ifdef __cplusplus
}
#endif
#endif
