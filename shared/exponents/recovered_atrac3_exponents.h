#ifndef SONY_RECOVERED_ATRAC3_EXPONENTS_H
#define SONY_RECOVERED_ATRAC3_EXPONENTS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Actual +0x461580: one coarse logarithmic estimate for each four samples.
 * All arithmetic is on IEEE binary32 representations, including the original
 * unsigned out-of-range-to-zero rule. This is an allocation input, not the
 * final choice of 32 encoded scale factors. Buffers must not overlap. */
void sony_recovered_atrac3_exponents(const float spectrum[1024],
                                    uint32_t estimates[256]);
/* Actual +0x463fb0 returns the same estimate for one four-value group. */
uint32_t sony_recovered_atrac3_exponent4(const float spectrum[4]);
#ifdef __cplusplus
}
#endif
#endif
