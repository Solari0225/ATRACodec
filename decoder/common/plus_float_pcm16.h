#ifndef ATRACODEC_PLUS_FLOAT_PCM16_H
#define ATRACODEC_PLUS_FLOAT_PCM16_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Internal ordinarySony plus output conversion,2048float32 per channel
 * to4096interleaved signed16 values. No additional amplitude scaling.
 * Reproduces nearest-even FISTP32 (including integer-indefinite on invalid
 * input) then clamps to[-32768,+32767]. Host rounding cannot change it.
 * Returns0 or-1 for null pointers without writing output. */
int atracodec_plus_float_pcm16(const float *left,const float *right,int16_t *output);
#ifdef __cplusplus
}
#endif
#endif
