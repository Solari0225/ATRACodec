#ifndef SONY_RECOVERED_PLUS_COST_H
#define SONY_RECOVERED_PLUS_COST_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original3ee530 cost on already-packed mathematical symbols. n counts input
 * quantized coefficients, descriptor1..112 chooses the observed Huff table.
 * Original cost loops are unrolled4symbols; valid QU sizes0/16/32/64/128
 * satisfy n%(coefficient_group*4)==0. No sign bits are included here.
 * Width0 table holes retain original cost0, rather than inventing a codeword;
 * the separate actual serializer still rejects unavailable tokens as needed.
 * Returns exact bit count, or-1 invalid independent API input.
 */
int32_t sony_recovered_plus_packed_cost(uint32_t descriptor_index,
    const int16_t *symbols,uint32_t n);
#ifdef __cplusplus
}
#endif
#endif
