#ifndef SONY_INDEPENDENT_PLUS_MDCT_H
#define SONY_INDEPENDENT_PLUS_MDCT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Actually executed original40e4ed SIMD per-band window/fold and MDCT.
 * state[0..63] is folded overlap and is updated; state[64..191] contains
 *128time samples and is preserved. Output128spectrum floats is overwritten.
 * window_flag chooses one of2original window coefficient sets (nonzero=>1).
 * gain_scale and reverse_flag are upstream already-selected parameters.
 * No arbitrary native record bytes or original code pointers are exposed.
 * Finite normal/signed-zero binary32 inputs; no FMA, buffers nonoverlapping.
 * This does not select gain/window flags or buffer the enclosing PCM stream.
 */
int sony_recovered_plus_mdct(float state[192],float output[128],
    uint32_t window_flag,float gain_scale,uint32_t reverse_flag);
#ifdef __cplusplus
}
#endif
#endif
