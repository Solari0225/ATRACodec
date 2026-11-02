#ifndef ATRACODEC_PLUS_IMDCT_FLOAT_H
#define ATRACODEC_PLUS_IMDCT_FLOAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered Sony ordinary ATRAC3plus inverse transform and overlap for128
 * spectral samples, producing128 output samples and updating64 history words.
 * spectrum is scratch and is modified. All arrays must be distinct.
 * window_flag and reverse_flag are0 or1; gain_scale is a binary32 scalar.
 * Floating-point environment: round to nearest, binary32/binary64 operations,
 * no contraction. The codec entry point supplies the environment guard. */
void atracodec_plus_imdct_float(float spectrum[128], float output[128],
                              float history[64], int window_flag,
                              float gain_scale, int reverse_flag);

#ifdef __cplusplus
}
#endif
#endif
