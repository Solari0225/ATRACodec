#ifndef SONY_PLUS_TIME_DETECTION_H
#define SONY_PLUS_TIME_DETECTION_H
#include "recovered_plus_time_math.h"
#ifdef __cplusplus
extern "C" {
#endif
double sony_plus_time_log_measure(float value);
int sony_plus_time_coherence(float *const left[16],float *const right[16],uint32_t count,uint32_t bands,
    float contrast[16],float left_energy[16],float right_energy[16],uint32_t backend);
int sony_plus_time_detect(sony_plus_time_common *current,float *const input[32],int32_t order[16],
    float energy[2][16],uint32_t bands,uint32_t channels,uint32_t *evolving_flags,uint32_t backend);
int sony_plus_time_budget(const sony_plus_time_common *current,const float energy[2][16],uint32_t selection_mode,
    int32_t counts[2][16],const int32_t order[16],int32_t profile,uint32_t channels);
#ifdef __cplusplus
}
#endif
#endif
