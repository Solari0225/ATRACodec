#ifndef SONY_RECOVERED_ATRAC3_SPECTRUM_H
#define SONY_RECOVERED_ATRAC3_SPECTRUM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Given decided word lengths and scale factors, quantize the original
 * contiguous four-band/1024-float spectrum and emit its mode0 VLC payload.
 * Does not choose those parameters, perform analysis, or form a whole frame.
 * start_bit and *end_bit are absolute offsets within output.
 * Returns 0 on success, -1 for invalid parameters, -2 for insufficient capacity.
 * Like the original, word stores can modify up to 3 padding bytes beyond
 * the meaningful bitstream end. Supply a zero-filled destination for encoding.
 * Input/output must not overlap. On error the output may be partially written.
 */
int sony_recovered_atrac3_write_spectrum(const float spectrum[1024],
    const uint32_t word_lengths[32], const uint32_t scale_factors[32],
    uint32_t quantization_units, uint8_t *output, uint32_t capacity,
    uint32_t start_bit, uint32_t *end_bit);
#ifdef __cplusplus
}
#endif
#endif
