#ifndef SONY_RECONSTRUCTION_ATRAC3_QUANT_COST_H
#define SONY_RECONSTRUCTION_ATRAC3_QUANT_COST_H
#include <stdint.h>
#if defined(_WIN32) && defined(SONY_MATH_TEST_EXPORTS)
#define SONY_QUANT_API __declspec(dllexport)
#else
#define SONY_QUANT_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Recovered spectral quantization/VLC-cost callback, region+0x4618a0.
 * wl=1..7, sf=0..63; count is 8 or a positive multiple of 16, <=1024.
 * Returns the native estimated bit cost (including six SF bits), or -1 for
 * an invalid independent API argument. The original codec supplies finite
 * spectral samples. IEEE binary32 nearest rounding is required; no FMA.
 * This callback neither chooses wl/sf nor emits a complete ATRAC3 frame.
 */
SONY_QUANT_API int32_t sony_recovered_atrac3_quant_cost(
    uint32_t word_length, uint32_t scale_factor, uint32_t count,
    const float *samples);
#ifdef __cplusplus
}
#endif
#endif
