#ifndef SONY_PLUS_TIME_TONAL_H
#define SONY_PLUS_TIME_TONAL_H
#include "recovered_plus_time_math.h"
#ifdef __cplusplus
extern "C" {
#endif
int sony_plus_time_dual_dot(const float *a,const float *b,const float *c,const float *d,
    float *first,float *second,uint32_t count,uint32_t backend);
int sony_plus_time_triple_dot(const float *a,const float *b,const float *c,const float *d,
    const float *e,const float *f,float *first,float *second,float *third,uint32_t count,uint32_t backend);
double sony_plus_time_fit_coefficients(const float input[256],int32_t frequency,
    float *sine_coefficient,float *cosine_coefficient,uint32_t backend);
uint32_t sony_plus_time_coefficient_phase(float sine_coefficient,float cosine_coefficient,int32_t frequency);
int sony_plus_time_refine(const float input[256],int32_t peak,float *amplitude,int32_t *phase,int32_t *frequency,uint32_t backend);
int sony_plus_time_extract(const float input[256],sony_plus_time_common *common,sony_plus_time_band *band,
    uint32_t selection_mode,int32_t peak,uint32_t maximum,uint32_t backend);
int sony_plus_time_synthesize(const sony_plus_time_common *common,const sony_plus_time_band *band,
    float *output,int32_t start,uint32_t count,uint32_t invert,uint32_t channel,uint32_t backend);
/* First original argument is previous-slot synthesis at start128; the second
 * frame is current-slot synthesis at start0. Windows are applied in that order. */
int sony_plus_time_overlap(const sony_plus_time_common *previous,const sony_plus_time_band *previous_band,
    uint32_t previous_invert,const sony_plus_time_common *current,const sony_plus_time_band *current_band,
    uint32_t current_invert,float output[128],uint32_t channel,uint32_t backend);
int sony_plus_time_filter_notes(sony_plus_time_common *common,sony_plus_time_band *band,uint32_t mode);
int sony_plus_time_continue(const float *input,uint32_t start,uint32_t count,float initial_energy,
    float *previous_energy,int32_t *keep,uint32_t backend);
#ifdef __cplusplus
}
#endif
#endif
