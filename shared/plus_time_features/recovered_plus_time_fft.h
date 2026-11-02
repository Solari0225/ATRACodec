#ifndef SONY_PLUS_TIME_FFT_H
#define SONY_PLUS_TIME_FFT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int sony_plus_time_fft_transform(float *values,uint32_t count,uint32_t backend,double *nyquist_out);
int sony_plus_time_fft_power256(const float *input,uint32_t count,float *power,uint32_t backend);
int sony_plus_time_fft_power(const float *input,uint32_t stride,uint32_t count,float *power);
#ifdef __cplusplus
}
#endif
#endif
