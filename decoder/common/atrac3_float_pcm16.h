#ifndef ATRACODEC_ATRAC3_FLOAT_PCM16_H
#define ATRACODEC_ATRAC3_FLOAT_PCM16_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Internal original ordinaryATRAC3 output conversion. Each input has1024
 * binary32 values; output has2048 interleaved signed16 values. Nearest-even
 * IEEE rounding is supplied by the public ATRACodec floating environment.
 * Sony clips symmetrically to[-32767,+32767] before the bias conversion.
 * Returns0, or-1 for null input/output without a write. */
int atracodec_atrac3_float_pcm16(const float *left,const float *right,int16_t *output);
#ifdef __cplusplus
}
#endif
#endif
