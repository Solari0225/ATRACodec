#ifndef SONY_RECOVERED_ATRAC3_TONAL_H
#define SONY_RECOVERED_ATRAC3_TONAL_H
#include "../sideinfo/recovered_atrac3_sideinfo.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original +0x464400 chooses a tonal component SF by examining the original
 * three neighboring candidates, reconstruction error and even-coefficient
 * scale folding. Input always supplies4floats, even when count<4.
 * *scale_factor initially holds the current value and is updated only when
 * the original strict error criterion improves. Valid WL2..7, count1..4.
 * This is component-local selection, not detection/grouping of tonal peaks.
 * Requires default round-to-nearest arithmetic, with FMA contraction disabled.
 */
int sony_recovered_atrac3_select_tonal_sf(const float input[4],uint32_t wl,
                                        uint32_t count,uint32_t *scale_factor);
/* +0x464340: select SF, quantize count coefficients into masked symbols,
 * subtract their reconstructed tonal contribution from input in place,
 * and return the original12-bit component overhead plus mode0 VLC lengths.
 * Component position and unused symbols are preserved. Discard on error.
 * This does not select the component's position, word length or group. */
int32_t sony_recovered_atrac3_quantize_tonal(float input[4],
    sony_atrac3_tonal_component *component,uint32_t wl,uint32_t count);
/* +0x464e20: collect original thresholded peak candidates, skipping the next
 * three coefficients after each hit. Returns count, or-1 on overflow/invalid
 * arguments; already-written positions survive an overflow. begin<end<=1024.
 * This primitive does not choose its threshold SF or the frame's tonal groups. */
int32_t sony_recovered_atrac3_collect_tonal_peaks(const float spectrum[1024],
    uint32_t sf,uint32_t begin,uint32_t end,uint32_t *positions,uint32_t capacity);
/* +0x464610: return a previously removed component into its spectrum slots
 * during refinement. Original fixed-code-width sign decoding is defined for
 * WL3,5,7; other mode1 book slots were observed null and fail explicitly.
 * count1..4; component position+count<=1024. Unlike the subtraction kernel,
 * this original path retains its reciprocal in binary64 until each store. */
int sony_recovered_atrac3_restore_tonal(float spectrum[1024],
    const sony_atrac3_tonal_component *component,uint32_t wl,uint32_t count);
#ifdef __cplusplus
}
#endif
#endif
