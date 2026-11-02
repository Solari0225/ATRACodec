#ifndef SONY_INDEPENDENT_PLUS_FFT_H
#define SONY_INDEPENDENT_PLUS_FFT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original analysis real FFT. n is16,32,64,128or256. Output packs DC and
 * Nyquist into a[0],a[1], followed by complex positive-frequency pairs. */
int sony_recovered_plus_real_fft(float *a,uint32_t n);
/* Read n samples at the supplied positive stride. Return n/2+1 powers,
 * including DC and Nyquist. The normal window analysis uses(128,1),(64,2). */
int sony_recovered_plus_power_spectrum(const float *input,uint32_t stride,
    uint32_t n,float *power);
#ifdef __cplusplus
}
#endif
#endif
