#ifndef SONY_RECOVERED_PLUS_QUANTIZATION_H
#define SONY_RECOVERED_PLUS_QUANTIZATION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original 403870/40f7bd math. Inputs are finite floats (original normalization
 * can exceed +/-1). Nonfinite inputs are rejected by this independent API.
 * WL=1..7; threshold_index=0..15; factor finite and nonnegative (including
 * original SSE-produced negative zero); N is 0..128,
 * a multiple of 4. Nonzero simd selects original SSE rounding/deadzone behavior.
 * Scalar strict threshold comparison and double multiply/add are preserved.
 * SIMD uses float multiply, inclusive threshold comparison, nearest-even round.
 * Caller uses nearest-even FP rounding (the official default). Returns 0/-1. */
int32_t sony_recovered_plus_quantize(const float *input,int16_t *output,
    uint32_t word_length,uint32_t threshold_index,float threshold_factor,
    uint32_t n,uint32_t simd);
/* Original int16 absolute/count and symbol packing children. N multiple of 16.
 * Group=1,2,4. For group>1, bits*group<=8; bits=1..6. SIMD ignores explicit
 * mask even for group1 and uses the original bit-width mask table. */
int32_t sony_recovered_plus_abs_count(const int16_t *input,int16_t *output,uint32_t n);
int32_t sony_recovered_plus_pack(const int16_t *input,uint32_t n,uint32_t group,
    uint32_t bits,uint32_t mask,int16_t *output,uint32_t simd);
/* Complete original 3edf60 float -> four short-table candidate costs, including
 * extra sign bits. Cost output is four adjacent int16 words, not the original
 * matrix base; the caller places them at row (QU+8), stride 8 int16 words.
 * bank=0..1. N=0,16,32,64,128. Does not allocate or select the winning candidate. */
int32_t sony_recovered_plus_quantization_cost(uint32_t bank,uint32_t word_length,
    uint32_t threshold_index,float threshold_factor,uint32_t n,const float *input,
    uint32_t simd,int16_t costs[4]);
uint32_t sony_recovered_plus_short_book(uint32_t bank,uint32_t word_length,uint32_t candidate);
#ifdef __cplusplus
}
#endif
#endif
