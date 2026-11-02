#ifndef ATRACODEC_ATRAC3_FLOAT_FILTER_H
#define ATRACODEC_ATRAC3_FLOAT_FILTER_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Internal Sony-derived float filter, observed entry466200.
 * buffer contains1162float32 values. The routine writes output[0..1023]
 * and intermediate[1024..1069]; remaining92lookahead values are preserved.
 * Requires IEEEbinary32, nearest rounding, gradual underflow, and disabled
 * FMA contraction; the enclosing ATRACodec public boundary supplies this.
 * No SIMD alignment beyond ordinary float storage is required.
 * Returns0; invalid null/short buffer returns-1 without a write.
 * This leaf is not a complete decoder and is not public ABI.
 */
int atracodec_atrac3_float_filter(float *buffer,size_t float_count);
#ifdef __cplusplus
}
#endif
#endif
