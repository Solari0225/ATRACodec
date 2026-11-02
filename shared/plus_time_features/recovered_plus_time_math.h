#ifndef SONY_RECOVERED_PLUS_TIME_MATH_H
#define SONY_RECOVERED_PLUS_TIME_MATH_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_time_note {
    uint32_t amplitude,fine_amplitude,phase,frequency;
} sony_plus_time_note;
/* Same numerical fields as the original40B band. first_note is an index in
 * the common48-note array, never a recovered native pointer. */
typedef struct sony_plus_time_band {uint32_t envelope[8],count,first_note;} sony_plus_time_band;
typedef struct sony_plus_time_common {
    uint32_t present,amplitude_mode,bands;
    /* next_note is a retained historical field name for the reserved pool
     * word at+0x30c. This graph never consumes it as an index or pointer. */
    sony_plus_time_note notes[48];uint32_t next_note;
    uint32_t flags[3][18];
} sony_plus_time_common;
void sony_plus_time_clear(sony_plus_time_common*,sony_plus_time_band[16],uint32_t bands);
/* op0 subtraction, op1 half-sum, op2 half-difference. Original scalar paths
 * touch floor(count/4)*4 entries; SIMD semantics touch ceil(count/4)*4. */
int sony_plus_time_mix(const float *left,const float *right,float *output,
    uint32_t count,uint32_t op,uint32_t backend);
uint32_t sony_plus_time_amplitude_code(float amplitude);
uint32_t sony_plus_time_phase_code(int32_t phase);
uint32_t sony_plus_time_expand_phase(uint32_t phase);
int sony_plus_time_remove_tone(const float input[256],float output[256],
    float amplitude,int32_t phase,int32_t frequency,uint32_t start,
    uint32_t end,uint32_t backend);
int sony_plus_time_sort_power(float *values,int32_t *indices,uint32_t count);
int sony_plus_time_sort_frequency(int32_t *values,int32_t *indices,uint32_t count);
int32_t sony_plus_time_largest_positive(const float *values,uint32_t count);
double sony_plus_time_peak_ratio(const float *values,uint32_t count);
double sony_plus_time_dot(const float *left,const float *right,uint32_t count,uint32_t backend);
#ifdef __cplusplus
}
#endif
#endif
