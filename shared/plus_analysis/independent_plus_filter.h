#ifndef SONY_INDEPENDENT_PLUS_FILTER_H
#define SONY_INDEPENDENT_PLUS_FILTER_H
#ifdef __cplusplus
extern "C" {
#endif
/* Recovered actually executed original40ec4d sixteen-band analysis/PQF.
 * One channel:2048binary32 input samples,368binary32 history,16x128output.
 * The portable output is compact band-major. The original stores the same
 *128samples at the front of each0x1100-byte band record; gaps are not DSP
 * input or output. History is updated with input[1680..2047]. Buffers must
 * not overlap. IEEE binary32 with separate add/sub/multiply, no contraction.
 * This is a full filter stage, not gain/MDCT or a complete plus encoder.
 */
int sony_recovered_plus_filter(const float input[2048],float history[368],
    float output[16][128]);
#ifdef __cplusplus
}
#endif
#endif
